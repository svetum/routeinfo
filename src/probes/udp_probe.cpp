#include <udp_probe.h>

#include <istream>
#include <iostream>
#include <ostream>
#include <icmp_header.hpp>
#include <ipv4_header.hpp>
#include <udp_header.hpp>
#include <boost/bind/bind.hpp>


udp_probe::udp_probe(boost::asio::io_context& io_context, const char* destination) : 
	raw_socket_(io_context, raw::endpoint(raw::v4(), 12345)),
	receive_socket_(io_context, boost::asio::ip::icmp::v4()), 
	receive_timeout_(io_context)
{	
	remote_end_point_ = std::string(destination);
	remote_end_point_port_ = 33434;
	ttl_ = 0;
}

void udp_probe::start() 
{
	start_receive();
	send_packet();
}

void udp_probe::send_packet()
{
	udp_header udp;
	udp.source_port(12345);
	udp.destination_port(++remote_end_point_port_);
	udp.length(udp.size() + sizeof(udp_payload_));
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
	ip.time_to_live(++ttl_);
	// change to get available IPv4 Endpoint !!!
	ip.source_address(boost::asio::ip::address::from_string("192.168.178.35").to_v4());
	ip.destination_address(boost::asio::ip::address::from_string(remote_end_point_).to_v4());
	ip.protocol(IPPROTO_UDP);	
	ip.calculate_checksum();

	boost::array<boost::asio::const_buffer, 3> buffers = {{
		boost::asio::buffer(ip.data()),
		boost::asio::buffer(udp.data()),
		boost::asio::buffer(udp_payload_, sizeof(udp_payload_))
	}};
	
	raw_socket_.async_send_to(buffers, 
		raw::endpoint(boost::asio::ip::address::from_string(remote_end_point_), remote_end_point_port_),
		boost::bind(&udp_probe::handle_send, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
			
	timestamp_ = boost::asio::steady_timer::clock_type::now();

	receive_timeout_.expires_at(timestamp_ + boost::asio::chrono::seconds(5));
	receive_timeout_.async_wait(boost::bind(&udp_probe::handle_timeout, this, boost::placeholders::_1));	
}
		
void udp_probe::start_receive()
{
	receive_buffer_.consume(receive_buffer_.size());
	receive_socket_.async_receive(receive_buffer_.prepare(65536), boost::bind(&udp_probe::handle_receive, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void udp_probe::handle_send(const boost::system::error_code& error, std::size_t bytes_transferred) 
{
}

void udp_probe::handle_timeout(const boost::system::error_code& error) 
{
	if(!error) {
		// this need to be handled somehow ...
		std::cout << "Request timed out" << std::endl;
		send_packet();
				
	}
}
		
void udp_probe::handle_receive(const boost::system::error_code& error, std::size_t length) 
{
	
	ipv4_header received_ipv4_header_1, received_ipv4_header_2;
	icmp_header received_icmp_header;
	udp_header received_udp_header;
	
	receive_buffer_.commit(length);

	// debug(receive_buffer_, length);

	

	std::istream is(&receive_buffer_);
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
		boost::asio::chrono::steady_clock::duration elapsed = now - timestamp_;
		std::cout << +ttl_ << ": " 
			<< received_ipv4_header_1.source_address().to_string()
			<< ", time = "
			<< boost::asio::chrono::duration_cast<boost::asio::chrono::milliseconds>(elapsed).count()
			<< std::endl;
	
		receive_timeout_.cancel();
	}
	
	if(received_ipv4_header_1.source_address().to_string() != remote_end_point_)
	{
		start_receive();
		send_packet();
	}
}
		

void udp_probe::debug(const boost::asio::streambuf& buffer, std::size_t length)
{
	std::cout << "========================================================================================" << std::endl;
	std::cout << "Packet received ..." << std::endl;
	boost::asio::streambuf copy;
	buffer_copy(copy.prepare(length), buffer.data());
	copy.commit(length);
	std::istream is(&copy);
	/*
	uint8_t message[length];
	copy.sgetn(reinterpret_cast<char *>(message), length);
	std::cout << "==== Message Begin ====" << std::endl;
	print_message(message, length);
	std::cout << "==== Message End ====" << std::endl;
	*/
	ipv4_header received_ipv4_header_1, received_ipv4_header_2;
	icmp_header received_icmp_header;
	udp_header received_udp_header;
	is >> received_ipv4_header_1 >> received_icmp_header >> received_ipv4_header_2 >> received_udp_header;
	// to be deleted 
	
	std::cout << "=====" << std::endl;
	std::cout << received_ipv4_header_1.print();
	std::cout << received_icmp_header.print();
	std::cout << received_ipv4_header_2.print();
	std::cout << received_udp_header.print();
	std::cout << "=====" << std::endl;
	
	// to be deleted end
}
