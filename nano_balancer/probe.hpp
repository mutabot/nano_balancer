//          Copyright Michael Shmalko 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once;
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/atomic.hpp>
#include "helper.hpp"
#include <unordered_map>

#define QUEUE_CAPACITY 512

namespace nano_balancer
{
	class probe : public boost::enable_shared_from_this<probe>
	{
		typedef boost::shared_ptr<boost::thread> thread_t;
		typedef ip::tcp::socket socket_type;
		boost::asio::detail::socket_type upstream_socket_;
		const std::string& config_name_;
		boost::asio::io_service& io_service;
		boost::mutex mutex_;

		std::unordered_map<unsigned long, ip_node_type> all_nodes;
		boost::lockfree::queue<unsigned long> good_nodes;
		boost::atomic<size_t> good_nodes_count;

		void add_nodes(std::list<ip_node_type> nodes)
		{
			boost::mutex::scoped_lock lock(mutex_);
			for (auto node : nodes)
			{
				all_nodes.insert_or_assign(node.hash(), node);
			}
		}

		thread_t thread;
		boost::posix_time::seconds period;
		boost::asio::deadline_timer probe_timer;

	public:
		typedef boost::shared_ptr<probe> ptr_type;
		probe(boost::asio::io_service& ios, const std::string& config_file_name)
			: config_name_(config_file_name),
			io_service(ios),
			good_nodes(QUEUE_CAPACITY),
			good_nodes_count(0),
			period(boost::posix_time::seconds(5)),
			probe_timer(ios, period)
		{
			add_nodes(helper::parse_config(config_name_));
		}
		
		void start()
		{
			probe_timer.async_wait(boost::bind(&probe::on_timer, shared_from_this(), boost::asio::placeholders::error));
		}

		ip_node_type get_next_node()
		{
			boost::mutex::scoped_lock lock(mutex_);
			unsigned long hash;
			if (good_nodes.pop(hash))
			{
				if (good_nodes.empty())
				{
					good_nodes.push(hash);
				}
				auto node = all_nodes[hash];
				std::cout << "Next: " << node.ip() << ":" << node.port() << std::endl;
				return node;
			}
			auto node = all_nodes.begin()->second;
			std::cout << "Fallback: " << node.ip() << ":" << node.port() << std::endl;
			return node;
		}

	protected:
		void handle_connect(const boost::system::error_code& error, boost::shared_ptr<socket_type>& socket, ip_node_type& node)
		{
			if (!error)
			{
				std::cout << "Good: " << node.ip() << ":" << node.port() << std::endl;
				while (good_nodes_count++ < QUEUE_CAPACITY / all_nodes.size())
				{
					good_nodes.push(node.hash());
				}
			}
			else
			{
				std::cout << "Bad: " << node.ip() << ":" << node.port() << std::endl;
			}
			if (socket->is_open())
			{
				socket->close();
			}
		}

		void do_probe(ip_node_type node)
		{
			std::cout << "Probing: " << node.ip() << ":" << node.port() << std::endl;
			ip::tcp::endpoint ep(node.ip(), node.port());
			auto socket = boost::make_shared<socket_type>(io_service);
			socket->async_connect(
				ep,
				boost::bind(
					&probe::handle_connect,
					shared_from_this(),
					boost::asio::placeholders::error,
					socket,
					node
				)
			);
		}

		void on_timer(const boost::system::error_code& e)
		{
			probe_timer.expires_at(probe_timer.expires_at() + period);
			probe_timer.async_wait(boost::bind(&probe::on_timer, shared_from_this(), boost::asio::placeholders::error));

			{
				boost::mutex::scoped_lock lock(mutex_);
				for (auto node : all_nodes)
				{
					do_probe(node.second);
				}
			}
		}
	};
}
