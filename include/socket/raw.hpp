// raw.hpp
#ifndef RAW
#define RAW

#include <algorithm>
#include <iostream>
#include <numeric>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/cstdint.hpp>

/// @brief raw socket provides the protocol for raw socket.
class raw
{
	public:
		///@brief The type of a raw endpoint.
		typedef boost::asio::ip::basic_endpoint<raw> endpoint;

		///@brief The raw socket type.
		typedef boost::asio::basic_raw_socket<raw> socket;

		///@brief The raw resolver type.
		typedef boost::asio::ip::basic_resolver<raw> resolver;

		///@brief Construct to represent the IPv4 RAW protocol.
		static raw v4()
		{
			return raw(IPPROTO_RAW, PF_INET);
		}

		///@brief Construct to represent the IPv6 RAW protocol.
		static raw v6()
		{
			return raw(IPPROTO_RAW, PF_INET6);
		}

		///@brief Default constructor.
		explicit raw()
		: protocol_(IPPROTO_RAW),
		  family_(PF_INET)
		{}

		///@brief Obtain an identifier for the type of the protocol.
		int type() const
		{
			return SOCK_RAW;
		}

		///@brief Obtain an identifier for the protocol.
		int protocol() const
		{
			return protocol_;
		}

		///@brief Obtain an identifier for the protocol family.
		int family() const
		{
			return family_;
		}

		///@brief Compare two protocols for equality.
		friend bool operator==(const raw& p1, const raw& p2)
		{
			return p1.protocol_ == p2.protocol_ && p1.family_ == p2.family_;
		}

		/// Compare two protocols for inequality.
		friend bool operator!=(const raw& p1, const raw& p2)
		{
			return !(p1 == p2);
		}

	private:
		explicit raw(int protocol_id, int protocol_family)
		: protocol_(protocol_id),
		family_(protocol_family)
		{}

		int protocol_;
		int family_;
};

#endif

