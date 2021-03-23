#ifndef ICMP_TX
#define ICMP_TX

#include <boost/asio.hpp>
#include <boost/random.hpp>
#include <raw.hpp>

class icmp_tx
{
	public:
		
		icmp_tx(boost::asio::io_context& io_context, const char* destination, uint16_t port, uint8_t hops, uint32_t number_of_packets, uint32_t send_interval, uint16_t payload_size);
		
		void start();

	private:
		
		void send_packet();
		
		void handle_send(const boost::system::error_code& error, std::size_t bytes_transferred);
	
		void handle_receive(const boost::system::error_code& error, size_t length);

		boost::asio::basic_raw_socket<raw> raw_socket_;
		std::string remote_end_point_;
		uint16_t remote_end_point_port_;
		uint8_t ttl_;
		uint32_t number_of_packets_to_send_;
		uint32_t send_interval_;
		int16_t payload_size_;

		boost::asio::ip::icmp::socket receive_socket_;
		boost::asio::streambuf receive_buffer_;

		boost::asio::io_service::strand strand_;
		boost::asio::steady_timer send_timer_; 
		boost::asio::steady_timer stats_timer_; 
		
		uint16_t identifier_;
		uint16_t sequence_number_;
		boost::random::mt19937 gen_;
		boost::asio::chrono::steady_clock::time_point timestamp_;
		boost::asio::ip::icmp::resolver icmp_resolver_;
			
};

#endif
