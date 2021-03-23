#include <udp_tx.h>

#include <istream>
#include <iostream>
#include <ostream>
#include <icmp_header.hpp>
#include <ipv4_header.hpp>
#include <udp_header.hpp>
#include <boost/bind/bind.hpp>


udp_tx::udp_tx(boost::asio::io_context& io_context, const char* destination, uint16_t port, uint8_t hops, uint32_t number_of_packets, uint32_t send_interval, uint16_t payload_size) : 
	strand_(io_context),
	raw_socket_(io_context, raw::endpoint(raw::v4(), 12345)),
	receive_socket_(io_context, boost::asio::ip::icmp::v4()), 
	send_timer_(io_context), 
	stats_timer_(io_context) 
{	
	remote_end_point_ = std::string(destination);
	remote_end_point_port_ = port;
	ttl_ = hops == 0 ? 255 : hops;
	number_of_packets_to_send_ = number_of_packets; 
	send_interval_ = send_interval;
	payload_size_ = payload_size;
}

void udp_tx::start() 
{
	receive_buffer_.consume(receive_buffer_.size());
	receive_socket_.async_receive(receive_buffer_.prepare(65536), 
		boost::bind(&udp_tx::handle_receive, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
	
	send_timer_.expires_after(boost::asio::chrono::milliseconds(send_interval_));
	send_timer_.async_wait(strand_.wrap(boost::bind(&udp_tx::send_packet, this)));	
	
}

void udp_tx::send_packet()
{
	if(--number_of_packets_to_send_ > 0) 
	{
		send_timer_.expires_after(boost::asio::chrono::milliseconds(send_interval_));
		send_timer_.async_wait(strand_.wrap(boost::bind(&udp_tx::send_packet, this)));
	}
	
	udp_header udp;
	udp.source_port(12345);
	udp.destination_port(remote_end_point_port_);
	udp.length(udp.size() + payload_size_);
	udp.checksum(0);
	
	ipv4_header ip;
	ip.version(4);
	ip.header_length(ip.size() / 4);
	ip.type_of_service(0);
	ip.total_length(ip.size() + udp.length());
	ip.identification(0);
	ip.dont_fragment(false);
	ip.more_fragments(false);
	ip.fragment_offset(0);
	ip.time_to_live(ttl_);
	// change to get available IPv4 Endpoint !!!
	ip.source_address(boost::asio::ip::address::from_string("192.168.178.35").to_v4());
	ip.destination_address(boost::asio::ip::address::from_string(remote_end_point_).to_v4());
	ip.protocol(IPPROTO_UDP);	
	ip.calculate_checksum();

	uint8_t udp_payload_[payload_size_] = {0};

	boost::array<boost::asio::const_buffer, 3> buffers = {{
		boost::asio::buffer(ip.data()),
		boost::asio::buffer(udp.data()),
		boost::asio::buffer(udp_payload_, sizeof(udp_payload_))
	}};
	
	raw_socket_.async_send_to(buffers, 
		raw::endpoint(boost::asio::ip::address::from_string(remote_end_point_), remote_end_point_port_),
		boost::bind(&udp_tx::handle_send, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
	//timestamp_ = boost::asio::steady_timer::clock_type::now();

}
		

void udp_tx::handle_send(const boost::system::error_code& error, std::size_t bytes_transferred) 
{
}

		
void udp_tx::handle_receive(const boost::system::error_code& error, std::size_t length) 
{
	std::cout << "packet received" << std::endl;
	ipv4_header received_ipv4_header_1, received_ipv4_header_2;
	icmp_header received_icmp_header;
	udp_header received_udp_header;
	
	receive_buffer_.commit(length);

	// debug(receive_buffer_, length);

	std::istream is(&receive_buffer_);
	
	
	receive_socket_.async_receive(receive_buffer_.prepare(65536), 
		boost::bind(&udp_tx::handle_receive, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
	
	
	
	is >> received_ipv4_header_1;
	if(received_ipv4_header_1.protocol() == ipv4_header::protocol::icmp) 
	{
		is >> received_icmp_header >> received_ipv4_header_2;
		if(received_ipv4_header_2.protocol() == ipv4_header::protocol::udp)
			is >> received_udp_header;
	}
	/*
	int qq;
	std::cin >> qq;
	*/
			
	if (is && (received_icmp_header.type() == icmp_header::time_exceeded || received_icmp_header.type() == icmp_header::destination_unreachable) && (received_udp_header.source_port() == 12345)) // && received_udp_header.destination_port() == remote_end_point_port_)) 
	{
		boost::asio::chrono::steady_clock::time_point now = boost::asio::chrono::steady_clock::now();
		boost::asio::chrono::steady_clock::duration elapsed = now - now;// - timestamp_;
		std::cout << +ttl_ << ": " 
			<< received_ipv4_header_1.source_address().to_string()
			<< ", time = "
			<< boost::asio::chrono::duration_cast<boost::asio::chrono::milliseconds>(elapsed).count()
			<< std::endl;
	
		//receive_timeout_.cancel();
	}
	
	//retries_ = 0;
	

}
		
