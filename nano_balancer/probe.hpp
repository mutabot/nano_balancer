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
#include <unordered_set>

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

		std::unordered_map<size_t, ip_node_type> all_nodes;
		std::unordered_set<size_t> good_nodes_set;
		boost::lockfree::queue<size_t> good_nodes_queue;


		void add_nodes(std::list<ip_node_type> nodes)
		{
			boost::mutex::scoped_lock lock(mutex_);
			for (auto node : nodes)
			{
				all_nodes.insert_or_assign(node.hash, node);
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
			size_t node_hash = 0;
			if (good_nodes_queue.pop(node_hash))
			{
				// return to queue for round robbin
				good_nodes_queue.push(node_hash);
				auto node = all_nodes.at(node_hash);
				std::cout << boost::this_thread::get_id() << "\tNext: " << node.address << ":" << node.port << std::endl;
			}
			auto node = all_nodes.begin()->second;
			std::cout << boost::this_thread::get_id() << "\tFallback: " << node.address << ":" << node.port << std::endl;
			return node;
		}

	protected:
		void handle_connect(const boost::system::error_code& error, boost::shared_ptr<socket_type>& socket, ip_node_type& node)
		{
			boost::mutex::scoped_lock lock(mutex_);
			if (!error)
			{
				std::cout << boost::this_thread::get_id() << "\tGood: " << node.address << ":" << node.port << std::endl;
				if (good_nodes_set.find(node.hash) == good_nodes_set.end())
				{
					good_nodes_set.insert(node.hash);
					good_nodes_queue.push(node.hash);
				}
				// do nothing if node already in good nodes collections
			}
			else
			{
				std::cout << boost::this_thread::get_id() << "\tBad: " << node.address << ":" << node.port << std::endl;
				if (good_nodes_set.find(node.hash) == good_nodes_set.end())
				{
					// remove from the queue
					size_t hash = 0;
					for (auto i = 0; i < good_nodes_set.size(); ++i)
					{
						if (!good_nodes_queue.pop(hash))
						{
							std::cerr << boost::this_thread::get_id() << "\tQueue is empty: " << node.address << ":" << node.port << std::endl;
							break;
						}
						if (hash != node.hash)
						{
							good_nodes_queue.push(hash);
						}
					}
					// remove from the good set
					good_nodes_set.erase(node.hash);
				}
			}
			if (socket->is_open())
			{
				socket->close();
			}
		}

		void do_probe(ip_node_type& node)
		{
			std::cout << boost::this_thread::get_id() << "\tProbing: " << node.address << ":" << node.port << std::endl;
			ip::tcp::endpoint ep(node.address, node.port);
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
			boost::mutex::scoped_lock lock(mutex_);
			for (auto pair : all_nodes)
			{
				do_probe(pair.second);
			}			
		}
	};
}
