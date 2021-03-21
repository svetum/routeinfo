// udp_Header.hpp

#ifndef UDP_HEADER
#define UDP_HEADER

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <boost/asio/ip/address_v4.hpp>
#include <utils.hpp>

/*
	Packet header for UDP - rfc768

	 0                   1                   2                   3
	 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-------+-------+-------+-------+-------+-------+-------+------+   ---
	|          source port          |      destination port        |    ^
	+-------+-------+-------+-------+-------+-------+-------+------+  8 bytes
	|            length             |          checksum            |    v
	+-------+-------+-------+-------+-------+-------+-------+------+   ---
	/                        data (if any)                         /
	+-------+-------+-------+-------+-------+-------+-------+------+
*/

class udp_header
{
	public:
		
		udp_header() 
		{ 
			std::fill(buffer_.begin(), buffer_.end(), 0); 
		}

		void source_port(uint16_t value)
		{ 
			buffer_[0] = (value >> 8) & 0xFF;
			buffer_[1] = value & 0xFF;
		}
  
		uint16_t source_port()
		{ 
			return (buffer_[0] << 8) | buffer_[1];
		}
  
  		void destination_port(uint16_t value) 
  		{ 
			buffer_[2] = (value >> 8) & 0xFF;
			buffer_[3] = value & 0xFF;
		}
		
		uint16_t destination_port()
		{ 
			return (buffer_[2] << 8) | buffer_[3];
		}
		
		void length(uint16_t value)
		{ 
			buffer_[4] = (value >> 8) & 0xFF;
			buffer_[5] = value & 0xFF;
		}
  
		uint16_t length()
		{ 
			return (buffer_[4] << 8) | buffer_[5];
		}
		
		void checksum(uint16_t value)
		{ 
			buffer_[6] = (value >> 8) & 0xFF;
			buffer_[7] = value & 0xFF;
		}

		uint16_t checksum()
		{ 
			return (buffer_[6] << 8) | buffer_[7];
		}

	public:

		std::size_t size()
		{ 
			return buffer_.size(); 
		}

		const boost::array<uint8_t, 8>& data() const 
		{
			return buffer_; 
		}

		friend std::istream& operator>>(std::istream& is, udp_header& header) { 
			return is.read(reinterpret_cast<char*>(header.buffer_.data()), 8); 
		}

		std::string print() {
			std::stringstream strm;
			strm << "UDP Header" << std::endl;
			strm << toHexFormat(buffer_[0]) << toHexFormat(buffer_[1]) << toHexFormat(buffer_[2]) << toHexFormat(buffer_[3]) << "| Source port: " << ((buffer_[0] << 8) | buffer_[1]) << ", Target port: " << ((buffer_[2] << 8) | buffer_[3]) << std::endl;
			strm << toHexFormat(buffer_[4]) << toHexFormat(buffer_[5]) << toHexFormat(buffer_[6]) << toHexFormat(buffer_[7]) << std::endl;
			return strm.str();
		}

	private:
		
		boost::array<uint8_t, 8> buffer_;
};

#endif
