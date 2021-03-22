#ifndef PROBES_ICMP_PROBE
#define PROBES_ICMP_PROBE

#include <boost/asio.hpp>
#include <boost/random.hpp>
#include <raw.hpp>

class icmp_probe
{
	public:
		
		icmp_probe(boost::asio::io_context& io_context, const char* destination);
		
		void start();

	private:
		
		void send_packet();
		
		void start_receive();
		
		void handle_send(const boost::system::error_code& error, std::size_t bytes_transferred);
		
		void handle_timeout(const boost::system::error_code& error);
		
		void handle_receive(const boost::system::error_code& error, size_t length);

		boost::asio::basic_raw_socket<raw> raw_socket_;
		boost::asio::ip::icmp::resolver icmp_resolver_;
		std::string remote_end_point_;
		boost::asio::chrono::steady_clock::time_point timestamp_;
		uint8_t ttl_;
		uint16_t identifier_;
		uint16_t sequence_number_;
		boost::random::mt19937 gen_;
				
		boost::asio::ip::icmp::socket receive_socket_;
		boost::asio::streambuf receive_buffer_;
		boost::asio::steady_timer receive_timeout_; 
		uint8_t retries_;
		
		void debug(const boost::asio::streambuf& buffer, std::size_t length);
};

#endif
