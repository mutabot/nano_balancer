//          Copyright Michael Shmalko 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once;
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/lockfree/queue.hpp>
#include "helper.hpp"
#include <unordered_map>
#include <unordered_set>
#include "logging.h"

namespace nano_balancer
{
	class probe : public boost::enable_shared_from_this<probe>
	{
		enum definitions
		{
			queue_capacity = 512,
			queue_multiplier = 128,
			probe_period_sec = 5
		};
		typedef boost::shared_ptr<boost::thread> thread_t;
		typedef ip::tcp::socket socket_type;
		boost::asio::detail::socket_type upstream_socket_;
		const std::string& config_name_;
		boost::asio::io_service& io_service;
		boost::mutex mutex_;		
		std::unordered_map<size_t, ip_node_type> all_nodes;
		std::unordered_set<size_t> good_nodes_set;
		boost::lockfree::queue<size_t> good_nodes_queue;
		volatile std::atomic<size_t> queue_size;
		logger_type& logger_;


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
		probe(logger_type& logger, boost::asio::io_service& ios, const std::string& config_file_name)
			: config_name_(config_file_name),
			io_service(ios),
			good_nodes_queue(queue_capacity),
			queue_size(0),
			period(boost::posix_time::seconds(probe_period_sec)),
			probe_timer(ios, boost::posix_time::millisec(1)), logger_(logger)
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
//				BOOST_LOG_SEV(logger_, trivial::info)  << "\tNext: " << node.address << ":" << node.port;
				return node;
			}
			auto node = all_nodes.begin()->second;
			BOOST_LOG_SEV(logger_, trivial::info)  << "\tFallback: " << node.address << ":" << node.port;
			return node;
		}

	protected:
		void add_good_node(ip_node_type& node)
		{
			boost::mutex::scoped_lock lock(mutex_);
			BOOST_LOG_SEV(logger_, trivial::info)  << "\tGood: " << node.address << ":" << node.port;

			if (good_nodes_set.find(node.hash) == good_nodes_set.end())
			{
				good_nodes_set.insert(node.hash);
				for (auto i = 0; i < queue_multiplier; ++i, ++queue_size) 
				{
					good_nodes_queue.push(node.hash);
				}
			}
			// do nothing if node already in good nodes collections
		}

		void remove_good_node(ip_node_type& node)
		{
			boost::mutex::scoped_lock lock(mutex_);
			BOOST_LOG_SEV(logger_, trivial::info)  << "\tBad: " << node.address << ":" << node.port;
			
			// check if node exists in good notes set
			if (good_nodes_set.find(node.hash) == good_nodes_set.end())
			{
				// purge the the queue
				size_t hash = 0;
				auto check_count = queue_size;
				for (auto i = 0; i < check_count && good_nodes_queue.pop(hash); ++i)
				{
					if (hash == node.hash)
					{
						--queue_size;
					}
					else
					{
						good_nodes_queue.push(hash);
					}
				}
				// remove from the good set
				good_nodes_set.erase(node.hash);
			}
		}

		void handle_connect(const boost::system::error_code& error, boost::shared_ptr<socket_type>& socket, ip_node_type& node)
		{
			if (!error)
			{
				add_good_node(node);
			}
			else
			{
				remove_good_node(node);
			}
			if (socket->is_open())
			{
				socket->close();
			}
		}

		void do_probe(ip_node_type& node)
		{
			BOOST_LOG_SEV(logger_, trivial::info)  << "\tProbing: " << node.address << ":" << node.port;
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
