//          Copyright Michael Shmalko 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#pragma once
#include <boost/asio/ip/address_v4.hpp>
#include <boost/functional/hash.hpp>

namespace nano_balancer
{
	struct ip_node_type;
	std::size_t hash_value(ip_node_type const& node);

	struct ip_node_type
	{		
		boost::asio::ip::address_v4 address;
		unsigned short port;
		size_t hash;
	
		ip_node_type(): port(0), hash(0)
		{
		}

		ip_node_type(boost::asio::ip::address_v4 address, unsigned short port) :
			address(address),
			port(port)
		{			
			hash = hash_value(*this);
		}

		bool operator==(ip_node_type const& other) const
		{
			return address == other.address && port == other.port;
		}
	};

	inline std::size_t hash_value(ip_node_type const& node)
	{
		std::size_t seed = 0;
		boost::hash_combine(seed, node.address.to_ulong());
		boost::hash_combine(seed, node.port);		
		return seed;
	}
};
