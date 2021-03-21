#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <istream>
#include <iostream>
#include <ostream>

#include <raw.hpp>
#include <icmp_header.hpp>
#include <ipv4_header.hpp>
#include <udp_header.hpp>

class RouteInfo 
{
	public:
		// constructor
		RouteInfo(boost::asio::io_context& io_context, const char* destination) : 
			icmpSocket(io_context, boost::asio::ip::icmp::v4()), 
			icmpReplyTimer(io_context),
			udpSocket(io_context, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 12345)),
			udpRawSocket(io_context, raw::endpoint(raw::v4(), 12345))
		{
			udpRemoteEndPort = 33434;
			udpDestinationAddress = std::string(destination);
			startReceive();
			sendPacket();
		}

	private:
		// send a packet to the destination
		void sendPacket() 
		{
			if(ttl > 1) return;
			// Create the UDP header.
			udp_header udp;
			udp.source_port(12345);
			udp.destination_port(++udpRemoteEndPort);
			udp.length(udp.size() + sizeof(udpPayload)); // Header + Payload
			udp.checksum(0); // Optional for IPv4
			// Create the IPv4 header.
			ipv4_header ip;
			ip.version(4);                   // IPv4
			ip.header_length(ip.size() / 4); // 32-bit words
			ip.type_of_service(0);           // Differentiated service code point
			auto total_length = ip.size() + udp.size() + sizeof(udpPayload);
			ip.total_length(total_length); // Entire message.
			ip.identification(0);
			ip.dont_fragment(false);
			ip.more_fragments(false);
			ip.fragment_offset(0);
			ip.time_to_live(++ttl);
			ip.source_address(boost::asio::ip::address::from_string("192.168.178.35").to_v4());
			ip.destination_address(boost::asio::ip::address::from_string(udpDestinationAddress).to_v4());
			ip.protocol(IPPROTO_UDP);	
			ip.calculate_checksum();

			// Gather up all the buffers and send through the raw socket.
			boost::array<boost::asio::const_buffer, 3> buffers = {{
				boost::asio::buffer(ip.data()),
				boost::asio::buffer(udp.data()),
				boost::asio::buffer(udpPayload, sizeof(udpPayload))
			}};
			udpRawSocket.async_send_to(buffers, 
				raw::endpoint(boost::asio::ip::address::from_string("8.8.8.8"), udpRemoteEndPort),
				boost::bind(&RouteInfo::handle_send, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
			
			// set the TTL for the IP packet
			//boost::asio::ip::unicast::hops option(++ttl);
			//udpSocket.set_option(option);
			// record the timestamp
			udpPacketSendTimestamp = boost::asio::steady_timer::clock_type::now();
			// create UDP remote end point
			//udpRemoteEndPoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(udpDestinationAddress), ++udpRemoteEndPort);
			// send the packet
			//udpSocket.async_send_to(boost::asio::buffer(udpPayload, sizeof(udpPayload)), udpRemoteEndPoint,
			//	boost::bind(&RouteInfo::handle_send, this,
			//	boost::asio::placeholders::error,
			//	boost::asio::placeholders::bytes_transferred));
			// start a timer - maximum time to wait for ICMP error message
			icmpReplyTimer.expires_at(udpPacketSendTimestamp + boost::asio::chrono::seconds(5));
			icmpReplyTimer.async_wait(boost::bind(&RouteInfo::handle_timeout, this, boost::placeholders::_1));
		}
		// start listening for ICMP packets
		void startReceive() 
		{
			// discard any data already in the buffer.
			icmpBuffer.consume(icmpBuffer.size());
			// wait for a reply - receive up to 64KB.
			icmpSocket.async_receive(icmpBuffer.prepare(65536), boost::bind(&RouteInfo::handle_receive, this, boost::placeholders::_2));
		}

		// timeout handle
		void handle_timeout(const boost::system::error_code& error) 
		{
			if(!error) {
				std::cout << "Request timed out" << std::endl;
				// Requests must be sent no less than one second apart.
				//timer_.expires_at(time_sent_ + chrono::seconds(1));
				//timer_.async_wait(boost::bind(&RouteInfo::start_send, this));
				// start listening for ICMP packets
				// startReceive();
				// send another packet with increased TTL
				sendPacket();
				
			}
		}
		
		// receive handle
		void handle_receive(std::size_t length) 
		{
			std::cout << "========================================================================================" << std::endl;
			std::cout << "Packet received ..." << std::endl;
			// The actual number of bytes received is committed to the buffer so that we
			// can extract it using a std::istream object.
			icmpBuffer.commit(length);
			
			// to be deleted
			boost::asio::streambuf debugCopy;
			buffer_copy(debugCopy.prepare(length), icmpBuffer.data());
			debugCopy.commit(length);
			std::istream isDebugCopy(&debugCopy);
			uint8_t message[length];
			debugCopy.sgetn(reinterpret_cast<char *>(message), length);
			
			std::cout << "==== Complete Message Begin ====" << std::endl;
			std::cout << "Length: " << length << std::endl;
			printHex(message, length);
			std::cout << "==== Complete Message End ====" << std::endl;
			// to be deleted end
			
			// decode the reply packet.
			std::istream is(&icmpBuffer);
			ipv4_header ipv4Header, ipv4Header2;
			icmp_header icmpHeader;
			udp_header udpHeader;
			// extract the headers from the buffer
			is >> ipv4Header;
			if(ipv4Header.protocol() == ipv4_header::protocol::icmp) {
				is >> icmpHeader >> ipv4Header2;
				if(ipv4Header2.protocol() == ipv4_header::protocol::udp)
					is >> udpHeader;
			}
			
			// to be deleted 
			std::cout << "=====" << std::endl;
			std::cout << ipv4Header.print();
			std::cout << icmpHeader.print();
			std::cout << ipv4Header2.print();
			std::cout << udpHeader.print();
			std::cout << "=====" << std::endl;
			// to be deleted end
			
			
			
			// all ICMP packets are captured
			// filter out only the replies that match the our identifier and
			// expected sequence number.
			if (is && (icmpHeader.type() == icmp_header::time_exceeded || icmpHeader.type() == icmp_header::destination_unreachable) && udpHeader.source_port() == 12345 && udpHeader.destination_port()) {
				// If this is the first reply, interrupt the five second timeout.
				/*if (num_replies_++ == 0)
					timer_.cancel();*/
				// Print out some information about the reply packet.
				boost::asio::chrono::steady_clock::time_point now = boost::asio::chrono::steady_clock::now();
				boost::asio::chrono::steady_clock::duration elapsed = now - udpPacketSendTimestamp;
				std::cout << +ttl << ": " << ipv4Header.source_address().to_string() 
				<< ", time = " << boost::asio::chrono::duration_cast<boost::asio::chrono::milliseconds>(elapsed).count() << std::endl;
				// cancel the timer
				icmpReplyTimer.cancel();
			}
			if(ipv4Header.source_address().to_string() != udpDestinationAddress)
			{
				// start listening for ICMP packets
				startReceive();
				// send another packet with increased TTL
				sendPacket();
			}
		}
		
		// send handle
		void handle_send(const boost::system::error_code& /*error*/, std::size_t /*bytes_transferred*/) 
		{
			//std::cout << "Probe packet sent." << std::endl;
		}

		// get the process id
		static unsigned short get_identifier() {
			return static_cast<unsigned short>(::getpid());
		}

		// helper function for debugging
		void printHex(uint8_t *message, int length) {
			for(int i = 0; i < length; ++i) {
				std::cout << toHexFormat(message[i]);
				if((i +1) % 4 == 0) std::cout << std::endl;
			}
		}

		// ICMP variables
		boost::asio::ip::icmp::socket icmpSocket;
		boost::asio::streambuf icmpBuffer;
		boost::asio::steady_timer icmpReplyTimer; 
		
		
		// UDP variables
		boost::asio::ip::udp::socket udpSocket;
		boost::asio::ip::udp::endpoint udpRemoteEndPoint;
		uint16_t udpRemoteEndPort;
		std::string udpDestinationAddress;
		boost::asio::chrono::steady_clock::time_point udpPacketSendTimestamp;		
		uint8_t ttl = 0;

		boost::asio::basic_raw_socket<raw> udpRawSocket;

		
		// udp payload "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_"
		uint8_t udpPayload[32] = {
			0x40, 0x41, 0x42, 0x43, 
			0x44, 0x45, 0x46, 0x47, 
			0x48, 0x49, 0x4a, 0x4b, 
			0x4c, 0x4d, 0x4e, 0x4f, 
			0x50, 0x51, 0x52, 0x53, 
			0x54, 0x55, 0x56, 0x57, 
			0x58, 0x59, 0x5a, 0x5b, 
			0x5c, 0x5d, 0x5e, 0x5f
		};
};

int main(int argc, char* argv[])
{
	try
	{
		if (argc != 2)
		{
			std::cerr << "Usage: ping <host>" << std::endl;
			std::cerr << "(You may need to run this program as root.)" << std::endl;
			return 1;
		}

		boost::asio::io_context io_context;
		RouteInfo routeInfo(io_context, argv[1]);
		io_context.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}
}
/*
char packet_bytes[] = {
  0xdc, 0x39, 0x6f, 0xb9, 
  0x35, 0x20, 
  0x00, 0x15, 0x5d, 0x17, 
  0x68, 0x04, 0x08, 0x00, 
  0x45, 0x00, 0x00, 0x3c, IPv4 4 -> 45 4- protocol, 5 -> 20 bytes header, 3C --> length 60
  0x52, 0xc3, 0x00, 0x00, -- header 2 --> identification 0x52c3
  0x02, 0x11, 0xe3, 0x12, -- header 3 --> time to live = 2, 0x11 UDP 
  0xc0, 0xa8, 0xb2, 0x23, -- header 4 --> src: 192.168.178.35
  0x08, 0x08, 0x08, 0x08, -- header 5 --> dst: 8.8.8.8
  0x93, 0x88, 0x82, 0x9d, UDP header --> 0x9388 Source Port = 37768, 0x829d Destination Port = 33437
  0x00, 0x28, 0x83, 0x15, -- header 2 --> 0x28 = length 40 (including header) 0x8315 is checksum
  0x40, 0x41, 0x42, 0x43, --> udp data "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_"
  0x44, 0x45, 0x46, 0x47, 
  0x48, 0x49, 0x4a, 0x4b, 
  0x4c, 0x4d, 0x4e, 0x4f, 
  0x50, 0x51, 0x52, 0x53, 
  0x54, 0x55, 0x56, 0x57, 
  0x58, 0x59, 0x5a, 0x5b, 
  0x5c, 0x5d, 0x5e, 0x5f
};
* */

/*


int main()
{
  boost::asio::io_service io_service;

  // Create all I/O objects.
  boost::asio::ip::udp::socket receiver(io_service,
    boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0));
  boost::asio::basic_raw_socket<raw> sender(io_service,
    raw::endpoint(raw::v4(), 0));

  const auto receiver_endpoint = receiver.local_endpoint();

  // Craft a UDP message with a payload 'hello' coming from
  // 8.8.8.8:54321
  const boost::asio::ip::udp::endpoint spoofed_endpoint(
    boost::asio::ip::address_v4::from_string("8.8.8.8"),
    54321);
  const std::string payload = "hello";

  // Create the UDP header.
  udp_header udp;
  udp.source_port(spoofed_endpoint.port());
  udp.destination_port(receiver_endpoint.port());
  udp.length(udp.size() + payload.size()); // Header + Payload
  udp.checksum(0); // Optioanl for IPv4

  // Create the IPv4 header.
  ipv4_header ip;
  ip.version(4);                   // IPv4
  ip.header_length(ip.size() / 4); // 32-bit words
  ip.type_of_service(0);           // Differentiated service code point
  auto total_length = ip.size() + udp.size() + payload.size();
  ip.total_length(total_length); // Entire message.
  ip.identification(0);
  ip.dont_fragment(true);
  ip.more_fragments(false);
  ip.fragment_offset(0);
  ip.time_to_live(4);
  ip.source_address(spoofed_endpoint.address().to_v4());
  ip.destination_address(receiver_endpoint.address().to_v4());
  ip.protocol(IPPROTO_UDP);
  calculate_checksum(ip);

  // Gather up all the buffers and send through the raw socket.
  boost::array<boost::asio::const_buffer, 3> buffers = {{
    boost::asio::buffer(ip.data()),
    boost::asio::buffer(udp.data()),
    boost::asio::buffer(payload)
  }};
  auto bytes_transferred = sender.send_to(buffers,
    raw::endpoint(receiver_endpoint.address(), receiver_endpoint.port()));
  assert(bytes_transferred == total_length);

  // Read on the reciever.
  std::vector<char> buffer(payload.size(), '\0');
  boost::asio::ip::udp::endpoint sender_endpoint;
  bytes_transferred = receiver.receive_from(
      boost::asio::buffer(buffer), sender_endpoint);

  // Verify.
  assert(bytes_transferred == payload.size());
  assert(std::string(buffer.begin(), buffer.end()) == payload);
  assert(spoofed_endpoint == sender_endpoint);

  // Print endpoints.
  std::cout <<
    "Actual sender endpoint: " << sender.local_endpoint() << "\n"
    "Receiver endpoint: " << receiver.local_endpoint() << "\n"
    "Receiver's remote endpoint: " << sender_endpoint << std::endl;
}
* 
*/ 
