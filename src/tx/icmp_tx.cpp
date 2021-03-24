#include <icmp_tx.h>

#include <istream>
#include <iostream>
#include <ostream>
#include <icmp_header.hpp>
#include <ipv4_header.hpp>
#include <udp_header.hpp>
#include <boost/bind/bind.hpp>


icmp_tx::icmp_tx(boost::asio::io_context& io_context, const char* destination, uint8_t hops, uint32_t number_of_packets, uint32_t send_interval, uint16_t payload_size) : 
	strand_(io_context),
	raw_socket_(io_context, raw::endpoint(raw::v4(), 0)),
	receive_socket_(io_context, boost::asio::ip::icmp::v4()), 
	send_timer_(io_context), 
	stats_timer_(io_context),
	icmp_resolver_(io_context)
{	
	remote_end_point_ = std::string(destination);
	ttl_ = hops == 0 ? 255 : hops;
	number_of_packets_to_send_ = number_of_packets; 
	send_interval_ = send_interval;
	payload_size_ = payload_size;
	counter_ = 0;
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
	std::cout << "sendigna packet " << std::endl;
	icmp_header icmp;
	icmp.type(icmp_header::echo_request);
	icmp.code(0);
	identifier_ = 1;
	icmp.identifier(identifier_);
	sequence_number_ = 1;
	icmp.sequence_number(sequence_number_);
	std::string body = "";
	std::vector<uint8_t> p{0x1B, 0x1B, 0x1B, 0x1B};
	//icmp.calculate_checksum(body.begin(), body.end());
	icmp.calculate_checksum(p.begin(), p.end());
	ipv4_header ip;
	ip.version(4);
	ip.header_length(ip.size() / 4);
	ip.type_of_service(0);
	ip.total_length(ip.size() + icmp.size() + 4);
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

	boost::array<boost::asio::const_buffer, 3> buffers = {{
		boost::asio::buffer(ip.data()),
		boost::asio::buffer(icmp.data()),
		boost::asio::buffer(p)
	}};
	
	raw_socket_.async_send_to(buffers, 
		raw::endpoint(boost::asio::ip::address::from_string(remote_end_point_), 0),
		boost::bind(&icmp_tx::handle_send, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
	//timestamp_ = boost::asio::steady_timer::clock_type::now();
}
		

void icmp_tx::handle_send(const boost::system::error_code& error, std::size_t bytes_transferred) 
{
	std::cout << "packet send" << std::endl;
}

		
void icmp_tx::handle_receive(const boost::system::error_code& error, std::size_t length) 
{
	std::cout << "=========================" << ++counter_<< std::endl;
	
	ipv4_header received_ipv4_header_1, received_ipv4_header_2;
	icmp_header received_icmp_header_1, received_icmp_header_2;
	icmp_payload received_icmp_playload;
	receive_buffer_.commit(length);
	debug(receive_buffer_, length);
	

	std::istream is(&receive_buffer_);
	
	receive_socket_.async_receive(receive_buffer_.prepare(65536), 
		boost::bind(&icmp_tx::handle_receive, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
		
	is >> received_ipv4_header_1;
	if(received_ipv4_header_1.protocol() == ipv4_header::protocol::icmp) 
	{
		is >> received_icmp_header_1;
		if(received_icmp_header_1.type() == icmp_header::icmp_type::time_exceeded) 
		{
			is >> received_ipv4_header_2 >> received_icmp_header_2;
		}
		// TODO -> fix this for the time exceeded case ...
		uint16_t icmp_payload_size = received_ipv4_header_1.total_length() - received_ipv4_header_1.header_length() * 4 - received_icmp_header_1.size();
		if(icmp_payload_size)
		{
			received_icmp_playload.length(icmp_payload_size);
			is >> received_icmp_playload;
		}
	}

	std::cout << "ip2header version:; " << +received_ipv4_header_2.version() << std::endl;
	std::cout << "icmp header type:; " << +received_icmp_header_1.type() << std::endl;
	std::cout << "icmp header identifier 1&2 ; " << received_icmp_header_1.identifier() << " " << identifier_ << std::endl;
	std::cout << "icmp header sequence 1&2 ; " << received_icmp_header_1.sequence_number() << " " << sequence_number_ << std::endl;
	std::cout << "=========================" << ++counter_<< std::endl;


			
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
		

void icmp_tx::debug(const boost::asio::streambuf& buffer, std::size_t length)
{
	std::cout << "========================================================================================" << std::endl;
	std::cout << "Packet received ..." << std::endl;
	boost::asio::streambuf copy, copy1;
	buffer_copy(copy.prepare(length), buffer.data());
	buffer_copy(copy1.prepare(length), buffer.data());
	copy.commit(length);
	copy1.commit(length);
	std::istream is(&copy);
	std::istream is1(&copy1);
	
	uint8_t message[length];
	
	copy.sgetn(reinterpret_cast<char *>(message), length);
	std::cout << "==== Message Begin ====" << std::endl;
	print_message(message, length);
	std::cout << "==== Message End ====" << std::endl;
	
	ipv4_header received_ipv4_header;
	icmp_header received_icmp_header;
	is1 >> received_ipv4_header >> received_icmp_header;

	// to be deleted 
	std::cout << "=====" << std::endl;
	std::cout << received_ipv4_header.print();
	std::cout << received_icmp_header.print();
	std::cout << "=====" << std::endl;
	// to be deleted end
}
