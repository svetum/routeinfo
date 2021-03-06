#ifndef PROTOCOL_IPV4_HEADER
#define PROTOCOL_IPV4_HEADER

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <boost/asio/ip/address_v4.hpp>
#include <utils.hpp>

/*
	Packet header for IPv4 - rfc791

	 0                   1                   2                   3
	 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-------+-------+-------+-------+-------+-------+-------+------+   ---
	|version|header |    type of    |    total length in bytes     |    ^
	|  (4)  | length|    service    |                              |    |
	+-------+-------+-------+-------+-------+-------+-------+------+    |
	|        identification         |flags|    fragment offset     |    |
	+-------+-------+-------+-------+-------+-------+-------+------+  20 bytes
	| time to live  |   protocol    |       header checksum        |    |
	+-------+-------+-------+-------+-------+-------+-------+------+    |
	|                      source IPv4 address                     |    |
	+-------+-------+-------+-------+-------+-------+-------+------+    |
	|                   destination IPv4 address                   |    v
	+-------+-------+-------+-------+-------+-------+-------+------+   ---
	/                       options (if any)                       /
	+-------+-------+-------+-------+-------+-------+-------+------+
*/

class ipv4_header
{
	public:

		enum protocol
		{ 
			icmp = 0x01,
			tcp = 0x06,
			udp = 0x11
		};
	
		ipv4_header() 
		{ 
			std::fill(buffer_.begin(), buffer_.end(), 0); 
			std::fill(options_.begin(), options_.end(), 0); 
		}

		void version(uint8_t value) 
		{
			buffer_[0] = (value << 4) | (buffer_[0] & 0x0F);
		}
		
		uint8_t version() 
		{
			return (buffer_[0] >> 4) & 0x0F;
		}
		
		void header_length(uint8_t value)
		{
			buffer_[0] = (value & 0x0F) | (buffer_[0] & 0xF0);
		}
		
		uint8_t header_length()
		{
			return buffer_[0] & 0x0F;
		}
		
		void type_of_service(uint8_t value)
		{ 
			buffer_[1] = value; 
		}
		
		uint8_t type_of_service()
		{ 
			return buffer_[1]; 
		}
		
		void total_length(uint16_t value)
		{ 
			buffer_[2] = (value >> 8) & 0xFF;
			buffer_[3] = value & 0xFF;
		}
  
		uint16_t total_length()
		{ 
			return (buffer_[2] << 8) | buffer_[3];
		}
  
		void identification(uint16_t value) 
		{ 
			buffer_[4] = (value >> 8) & 0xFF;
			buffer_[5] = value & 0xFF;
		}
		
		uint16_t identification() 
		{ 
			return (buffer_[4] << 8) | buffer_[5];
		}
		

		void dont_fragment(bool value)
		{
			buffer_[6] ^= (-value ^ buffer_[6]) & 0x40;
		}

		bool dont_fragment()
		{
			return buffer_[6] & 0x40;
		}

		void more_fragments(bool value)
		{
			buffer_[6] ^= (-value ^ buffer_[6]) & 0x20;
		}
		
		bool more_fragments()
		{
			return buffer_[6] & 0x20;
		}

		void fragment_offset(uint16_t value)
		{
			auto flags = static_cast<uint16_t>(buffer_[6] & 0xE0) << 8;
			buffer_[6] = (((value & 0x1FFF) | flags) >> 8) & 0xFF;
			buffer_[7] = value & 0xFF;
		}

		uint16_t fragment_offset()
		{
			return ((buffer_[6] << 8) | buffer_[7]) & 0x1FFF;
		}
		
		void time_to_live(uint8_t value)
		{ 
			buffer_[8] = value; 
		}
		
		uint8_t time_to_live()
		{ 
			return buffer_[8]; 
		}
		
		void protocol(uint8_t value) 
		{ 
			buffer_[9] = value; 
		}
		
		uint8_t protocol() 
		{ 
			return buffer_[9]; 
		}
		
		void checksum(uint16_t value) 
		{ 
			buffer_[10] = (value >> 8) & 0xFF;
			buffer_[11] = value & 0xFF;
		}
		
		uint16_t checksum() 
		{ 
			return (buffer_[10] << 8) | buffer_[11];
		}
		
		void source_address(boost::asio::ip::address_v4 value)
		{
			auto bytes = value.to_bytes();
			std::copy(bytes.begin(), bytes.end(), &buffer_[12]);
		}

		boost::asio::ip::address_v4 source_address()
		{
			return boost::asio::ip::address_v4({buffer_[12], buffer_[13], buffer_[14], buffer_[15]});
		}

		void destination_address(boost::asio::ip::address_v4 value)
		{
			auto bytes = value.to_bytes();
			std::copy(bytes.begin(), bytes.end(), &buffer_[16]);
		}

		boost::asio::ip::address_v4 destination_address()
		{
			return boost::asio::ip::address_v4({buffer_[16], buffer_[17], buffer_[18], buffer_[19]});
		}
		
		void calculate_checksum()
		{
			// Zero out the checksum
			checksum(0);
			// Checksum is the 16-bit one's complement of the one's complement sum of
			// all 16-bit words in the header.
			// Sum all 16-bit words.
			auto data = buffer_.data();
			auto sum = std::accumulate<uint16_t*, uint32_t>(
				reinterpret_cast<uint16_t*>(&data[0]),
				reinterpret_cast<uint16_t*>(&data[0] + buffer_.size()),
			0);
			// Fold 32-bit into 16-bits.
			while (sum >> 16)
			{
				sum = (sum & 0xFFFF) + (sum >> 16);
			}
			checksum(~sum);
		}

	public:

		std::size_t size() const
		{ 
			return buffer_.size(); 
		}

		const boost::array<uint8_t, 20>& data() const
		{ 
			return buffer_;
		}

		friend std::istream& operator>>(std::istream& is, ipv4_header& header) 
		{
			is.read(reinterpret_cast<char*>(header.buffer_.data()), 20);
			if (header.version() != 4)
				is.setstate(std::ios::failbit);
			std::streamsize options_length = header.header_length() * 4 - 20;
			if (options_length < 0 || options_length > 40) 
				is.setstate(std::ios::failbit);
			else {
				is.read(reinterpret_cast<char*>(header.options_.data()), options_length);
			}
			return is;
		}

		std::string print() {
			std::stringstream strm;
			strm << "IPv4 Header" << std::endl;
			strm << to_hex(buffer_[0]) << to_hex(buffer_[1]) << to_hex(buffer_[2]) << to_hex(buffer_[3]) << "| Header Length: " <<  +(buffer_[0] & 0xF) << ", Total Length: " << ((buffer_[2] << 8) | buffer_[3]) << std::endl;
			strm << to_hex(buffer_[4]) << to_hex(buffer_[5]) << to_hex(buffer_[6]) << to_hex(buffer_[7]) << std::endl;
			strm << to_hex(buffer_[8]) << to_hex(buffer_[9]) << to_hex(buffer_[10]) << to_hex(buffer_[11]) << "| TTL: " << +buffer_[8] << std::endl;
			strm << to_hex(buffer_[12]) << to_hex(buffer_[13]) << to_hex(buffer_[14]) << to_hex(buffer_[15]) << "| Src: " << +buffer_[12] << "."  << +buffer_[13] << "."  << +buffer_[14] << "."  << +buffer_[15] << std::endl;
			strm << to_hex(buffer_[16]) << to_hex(buffer_[17]) << to_hex(buffer_[18]) << to_hex(buffer_[19]) << "| Dst: " << +buffer_[16] << "."  << +buffer_[17] << "."  << +buffer_[18] << "."  << +buffer_[19] << std::endl;
			return strm.str();
		}

	private:

		boost::array<uint8_t, 20> buffer_;
		boost::array<uint8_t, 40> options_;
};

#endif
