// icmp_header.hpp

#ifndef ICMP_HEADER
#define ICMP_HEADER

#include <istream>
#include <ostream>
#include <algorithm>
#include <Utils.hpp>

/* 
	ICMP header for both IPv4 and IPv6 - rfc792

	 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 
	+---------------+---------------+-------------------------------+ 
	|     type      |     code      |           checksum            | 
	+---------------+---------------+-------------------------------+ 
	~          				rest of header                          ~ 
	+-------------------------------+-------------------------------+ 
*/

class icmp_header {
	
	public:
		
		enum 
		{ 
			echo_reply = 0, 
			destination_unreachable = 3, 
			source_quench = 4,
			redirect = 5, 
			echo_request = 8, 
			time_exceeded = 11, 
			parameter_problem = 12,
			timestamp_request = 13, 
			timestamp_reply = 14, 
			info_request = 15,
			info_reply = 16, 
			address_request = 17, 
			address_reply = 18 
		};

		icmp_header() 
		{ 
			std::fill(buffer_.begin(), buffer_.end(), 0); 
		}

		void type(uint8_t value)
		{ 
			buffer_[0] = value; 
		}

		uint8_t type()
		{ 
			return buffer_[0]; 
		}
		
		void code(uint8_t value)
		{ 
			buffer_[1] = value; 
		}
				
		uint8_t code()
		{ 
			return buffer_[1]; 
		}
	  
		void checksum(uint16_t value)
		{ 
			buffer_[2] = (value >> 8) & 0xFF;
			buffer_[3] = value & 0xFF;
		}
	  
		uint16_t checksum()
		{ 
			return (buffer_[2] << 8) | buffer_[3];
		}
		
		void identifier(uint16_t value)
		{ 
			buffer_[4] = (value >> 8) & 0xFF;
			buffer_[5] = value & 0xFF;
		}
			
		uint16_t identifier()
		{ 
			return (buffer_[4] << 8) | buffer_[5];
		}
		
		void sequence_number(uint16_t value)
		{ 
			buffer_[6] = (value >> 8) & 0xFF;
			buffer_[7] = value & 0xFF;
		}
		
		uint16_t sequence_number() const 
		{ 
			return (buffer_[6] << 8) | buffer_[7];
		}
	
		/*
		void calculate_checksum() {
			unsigned int sum = ((buffer_[0] << 8) | buffer_[1]) + ((buffer_[4] << 8) | buffer_[5]) + ((buffer_[6] << 8) | buffer_[7]);
			Iterator body_iter = body_begin;
			while (body_iter != body_end) {
			sum += (static_cast<unsigned char>(*body_iter++) << 8);
			if (body_iter != body_end)
				sum += static_cast<unsigned char>(*body_iter++);
			}
			sum = (sum >> 16) + (sum & 0xFFFF);
			sum += (sum >> 16);
			checksum(static_cast<uint16_t>(~sum));
		}
		*/ 
	public: 
	
		std::size_t size()
		{ 
			return buffer_.size(); 
		}

		const boost::array<uint8_t, 8>& data() const 
		{
			return buffer_; 
		}

		friend std::istream& operator>>(std::istream& is, icmp_header& header) { 
			return is.read(reinterpret_cast<char*>(header.buffer_.data()), 8); 
		}
		
		std::string print() {
			std::stringstream strm;
			strm << "ICMP Header" << std::endl;
			strm << toHexFormat(buffer_[0]) << toHexFormat(buffer_[1]) << toHexFormat(buffer_[2]) << toHexFormat(buffer_[3]) << std::endl;
			strm << toHexFormat(buffer_[4]) << toHexFormat(buffer_[5]) << toHexFormat(buffer_[6]) << toHexFormat(buffer_[7]) << std::endl;
			return strm.str();
		}
		
	private:
		
		boost::array<uint8_t, 8> buffer_;
};
#endif
