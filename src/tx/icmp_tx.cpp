#include <icmp_tx.h>

#include <istream>
#include <iostream>
#include <ostream>
#include <icmp_header.hpp>
#include <ipv4_header.hpp>
#include <udp_header.hpp>
#include <boost/bind/bind.hpp>


icmp_tx::icmp_tx(boost::asio::io_context& io_context, const char* destination, uint16_t port, uint8_t hops, uint32_t number_of_packets, uint32_t send_interval, uint16_t payload_size) : 
	strand_(io_context),
	raw_socket_(io_context, raw::endpoint(raw::v4(), 12345)),
	receive_socket_(io_context, boost::asio::ip::icmp::v4()), 
	send_timer_(io_context), 
	stats_timer_(io_context),
	icmp_resolver_(io_context)
{	
	remote_end_point_ = std::string(destination);
	remote_end_point_port_ = port;
	ttl_ = hops == 0 ? 255 : hops;
	number_of_packets_to_send_ = number_of_packets; 
	send_interval_ = send_interval;
	payload_size_ = payload_size;
}

void icmp_tx::start() 
{
	receive_buffer_.consume(receive_buffer_.size());
	receive_socket_.async_receive(receive_buffer_.prepare(65536), 
		boost::bind(&icmp_tx::handle_receive, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
	
	send_timer_.expires_after(boost::asio::chrono::milliseconds(send_interval_));
	send_timer_.async_wait(strand_.wrap(boost::bind(&icmp_tx::send_packet, this)));	
	
}

void icmp_tx::send_packet()
{
	if(--number_of_packets_to_send_ > 0) 
	{
		send_timer_.expires_after(boost::asio::chrono::milliseconds(send_interval_));
		send_timer_.async_wait(strand_.wrap(boost::bind(&icmp_tx::send_packet, this)));
	}
	
	icmp_header icmp;
	icmp.type(icmp_header::echo_request);
	icmp.code(0);
	identifier_ = gen_();
	icmp.identifier(identifier_);
	sequence_number_ = gen_();
	icmp.sequence_number(sequence_number_);
	std::string body = "";
	icmp.calculate_checksum(body.begin(), body.end());
	ipv4_header ip;
	ip.version(4);
	ip.header_length(ip.size() / 4);
	ip.type_of_service(0);
	ip.total_length(ip.size() + icmp.size());
	ip.identification(0);
	ip.dont_fragment(false);
	ip.more_fragments(false);
	ip.fragment_offset(0);
	ip.time_to_live(ttl_);
	// change to get available IPv4 Endpoint !!!
	ip.source_address(boost::asio::ip::address::from_string("192.168.178.35").to_v4());
	ip.destination_address(boost::asio::ip::address::from_string(remote_end_point_).to_v4());
	ip.protocol(ipv4_header::protocol::icmp);	
	ip.calculate_checksum();

	boost::array<boost::asio::const_buffer, 2> buffers = {{
		boost::asio::buffer(ip.data()),
		boost::asio::buffer(icmp.data())
	}};
	
	boost::asio::ip::icmp::resolver::query query(boost::asio::ip::icmp::v4(), remote_end_point_.c_str(), "");
    auto destination_ = *icmp_resolver_.resolve(query);
	
	raw_socket_.async_send_to(buffers, 
		raw::endpoint(boost::asio::ip::address::from_string(remote_end_point_), remote_end_point_port_),
		boost::bind(&icmp_tx::handle_send, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
	//timestamp_ = boost::asio::steady_timer::clock_type::now();

}
		

void icmp_tx::handle_send(const boost::system::error_code& error, std::size_t bytes_transferred) 
{
}

		
void icmp_tx::handle_receive(const boost::system::error_code& error, std::size_t length) 
{
ipv4_header received_ipv4_header_1, received_ipv4_header_2;
	icmp_header received_icmp_header_1, received_icmp_header_2;
	
	receive_buffer_.commit(length);

	receive_socket_.async_receive(receive_buffer_.prepare(65536), 
		boost::bind(&icmp_tx::handle_receive, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));

	std::istream is(&receive_buffer_);
	is >> received_ipv4_header_1;
	if(received_ipv4_header_1.protocol() == ipv4_header::protocol::icmp) 
	{
		is >> received_icmp_header_1;
		if(received_icmp_header_1.type() == icmp_header::icmp_type::time_exceeded) 
		{
			is >> received_ipv4_header_2 >> received_icmp_header_2;
		}	
	}
	/*
	int qq;
	std::cin >> qq;
	*/
			
	if (is && ((received_icmp_header_1.type() == icmp_header::icmp_type::time_exceeded && received_icmp_header_2.identifier() == identifier_ && received_icmp_header_2.sequence_number() == sequence_number_) || (received_ipv4_header_2.version() == 0 && received_icmp_header_1.type() == icmp_header::icmp_type::echo_reply && received_icmp_header_1.identifier() == identifier_ && received_icmp_header_1.sequence_number() == sequence_number_))) 
	{
		boost::asio::chrono::steady_clock::time_point now = boost::asio::chrono::steady_clock::now();
		boost::asio::chrono::steady_clock::duration elapsed = now - timestamp_;
		std::cout << +ttl_ << ": " 
			<< received_ipv4_header_1.source_address().to_string()
			<< ", time = "
			<< boost::asio::chrono::duration_cast<boost::asio::chrono::milliseconds>(elapsed).count()
			<< std::endl;
		
	}
	

}
		
