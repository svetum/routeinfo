#ifndef PROBES_UDP_PROBE
#define PROBES_UDP_PROBE

#include <boost/asio.hpp>
#include <raw.hpp>

class udp_probe
{
	public:
		
		udp_probe(boost::asio::io_context& io_context, const char* destination);
		
		void start();

	private:
		
		void send_packet();
		
		void start_receive();
		
		void handle_send(const boost::system::error_code& error, std::size_t bytes_transferred);
		
		void handle_timeout(const boost::system::error_code& error);
		
		void handle_receive(const boost::system::error_code& error, size_t length);

		boost::asio::basic_raw_socket<raw> raw_socket_;
		std::string remote_end_point_;
		uint16_t remote_end_point_port_;
		boost::asio::chrono::steady_clock::time_point timestamp_;
		uint8_t ttl_;
		uint8_t udp_payload_[32] =  
		{
			0x40, 0x41, 0x42, 0x43, 
			0x44, 0x45, 0x46, 0x47, 
			0x48, 0x49, 0x4a, 0x4b, 
			0x4c, 0x4d, 0x4e, 0x4f, 
			0x50, 0x51, 0x52, 0x53, 
			0x54, 0x55, 0x56, 0x57, 
			0x58, 0x59, 0x5a, 0x5b, 
			0x5c, 0x5d, 0x5e, 0x5f
		};
		
		boost::asio::ip::icmp::socket receive_socket_;
		boost::asio::streambuf receive_buffer_;
		boost::asio::steady_timer receive_timeout_; 
		
		void debug(const boost::asio::streambuf& buffer, std::size_t length);
		
};

#endif
