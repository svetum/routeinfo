#include <icmp_probe.h>

#include <istream>
#include <iostream>
#include <ostream>
#include <icmp_header.hpp>
#include <ipv4_header.hpp>
#include <boost/bind/bind.hpp>

icmp_probe::icmp_probe(boost::asio::io_context& io_context, const char* destination) : 
	raw_socket_(io_context, raw::endpoint(raw::v4(), 0)),
	icmp_resolver_(io_context),
	receive_socket_(io_context, boost::asio::ip::icmp::v4()), 
	receive_timeout_(io_context)
{	
	remote_end_point_ = std::string(destination);
	ttl_ = 0;
	identifier_ = 0;
	retries_ = 0;
}

void icmp_probe::start() 
{
	start_receive();
	send_packet();
}

void icmp_probe::send_packet()
{
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
	ip.time_to_live(++ttl_);
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
		raw::endpoint(boost::asio::ip::address::from_string(remote_end_point_), 0),
		boost::bind(&icmp_probe::handle_send, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
			
	timestamp_ = boost::asio::steady_timer::clock_type::now();

	receive_timeout_.expires_at(timestamp_ + boost::asio::chrono::seconds(5));
	receive_timeout_.async_wait(boost::bind(&icmp_probe::handle_timeout, this, boost::placeholders::_1));	
}
		
void icmp_probe::start_receive()
{
	receive_buffer_.consume(receive_buffer_.size());
	receive_socket_.async_receive(receive_buffer_.prepare(65536), boost::bind(&icmp_probe::handle_receive, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void icmp_probe::handle_send(const boost::system::error_code& error, std::size_t bytes_transferred) 
{
}

void icmp_probe::handle_timeout(const boost::system::error_code& error) 
{
	if(!error) {
		retries_++;
		// this need to be handled somehow ...
		std::cout << "Request timed out" << std::endl;
		if(retries_ < 3)
		{
			--ttl_;
		}
		send_packet();
	}
}
		
void icmp_probe::handle_receive(const boost::system::error_code& error, std::size_t length) 
{
	ipv4_header received_ipv4_header_1, received_ipv4_header_2;
	icmp_header received_icmp_header_1, received_icmp_header_2;
	
	receive_buffer_.commit(length);

	// debug(receive_buffer_, length);

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
		receive_timeout_.cancel();
	}
	
	retries_ = 0;
	
	if(received_ipv4_header_1.source_address().to_string() != remote_end_point_)
	{
		start_receive();
		send_packet();
	}
}
		

void icmp_probe::debug(const boost::asio::streambuf& buffer, std::size_t length)
{
	std::cout << "========================================================================================" << std::endl;
	std::cout << "Packet received ..." << std::endl;
	boost::asio::streambuf copy;
	buffer_copy(copy.prepare(length), buffer.data());
	copy.commit(length);
	std::istream is(&copy);
	
	uint8_t message[length];
	copy.sgetn(reinterpret_cast<char *>(message), length);
	std::cout << "==== Message Begin ====" << std::endl;
	print_message(message, length);
	std::cout << "==== Message End ====" << std::endl;
	
	ipv4_header received_ipv4_header;
	icmp_header received_icmp_header;
	is >> received_ipv4_header >> received_icmp_header;

	// to be deleted 
	std::cout << "=====" << std::endl;
	std::cout << received_ipv4_header.print();
	std::cout << received_icmp_header.print();
	std::cout << "=====" << std::endl;
	// to be deleted end
}
