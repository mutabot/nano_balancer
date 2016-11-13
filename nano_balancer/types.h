//          Copyright Michael Shmalko 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#pragma once
#include <boost/asio/ip/address_v4.hpp>

namespace nano_balancer
{
	class ip_node_type
	{		
		unsigned long hash_;
		boost::asio::ip::address_v4 address_;
		unsigned short port_;
	public:
		ip_node_type(): hash_(0), port_(0)
		{
		}

		ip_node_type(boost::asio::ip::address_v4 address, unsigned short port) :
			address_(address),
			port_(port)
		{
			hash_ = address_.to_ulong() + port;
		}

		unsigned long hash() const
		{
			return hash_;
		}

		boost::asio::ip::address_v4 ip() const
		{
			return address_;
		}

		unsigned short port() const
		{
			return port_;
		}

		bool operator<(const ip_node_type& other) const
		{
			return hash_ < other.hash_;
		};
	};

	struct ip_node_type_hash
	{
		size_t operator()(ip_node_type const& node) const noexcept
		{
			return std::hash<int>()(node.hash());
		};
	};

	struct ip_node_type_eq
	{
		bool operator()(const ip_node_type& lhs, const ip_node_type& rhs) const noexcept
		{
			return lhs.hash() == rhs.hash();
		};
	};
};
