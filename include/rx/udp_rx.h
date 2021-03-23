/*
#ifndef UDP_TX
#define UDP_TX

#include <boost/asio.hpp>
#include <raw.hpp>

class udp_tx
{
	public:
		
		udp_tx(boost::asio::io_context& io_context, const char* destination, uint16_t port, uint8_t hops, uint64_t number_of_packets);
		
		void start();

	private:
		
		void send_packet();
		
		void handle_send(const boost::system::error_code& error, std::size_t bytes_transferred);
	
		void handle_receive(const boost::system::error_code& error, size_t length);

		boost::asio::basic_raw_socket<raw> raw_socket_;
		std::string remote_end_point_;
		uint16_t remote_end_point_port_;
		uint8_t ttl_;
		uint64_t number_of_packets_to_send_;

		boost::asio::ip::icmp::socket receive_socket_;
		boost::asio::streambuf receive_buffer_;

		boost::asio::steady_timer send_timer_; 
		boost::asio::steady_timer stats_timer_; 
		
		
				
};

#endif
*/
