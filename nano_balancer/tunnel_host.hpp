// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once;
#include <iostream>
#include <string>

#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/function.hpp>
#include <boost/make_shared.hpp>
#include "types.h"

namespace nano_balancer
{
	namespace ip = boost::asio::ip;

	class tunnel : public boost::enable_shared_from_this<tunnel>
	{
	public:

		typedef ip::tcp::socket socket_type;
		typedef boost::shared_ptr<tunnel> ptr_type;

		explicit tunnel(boost::asio::io_service& ios)
			: downstream_(ios),
			upstream_(ios)
		{}

		socket_type& downstream_socket()
		{
			return downstream_;
		}

		socket_type& upstream_socket()
		{
			return upstream_;
		}

		void start(const ip::address_v4& upstream_host, unsigned short upstream_port)
		{
			upstream_.async_connect(
				ip::tcp::endpoint(upstream_host,
					upstream_port),
				boost::bind(&tunnel::handle_upstream_connect,
					shared_from_this(),
					boost::asio::placeholders::error));
		}

		void handle_upstream_connect(const boost::system::error_code& error)
		{
			if (!error)
			{
				upstream_.async_read_some(
					boost::asio::buffer(upstream_buffer_, buffer_size),
					boost::bind(&tunnel::handle_upstream_read,
						shared_from_this(),
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));

				downstream_.async_read_some(
					boost::asio::buffer(downstream_buffer_, buffer_size),
					boost::bind(&tunnel::handle_downstream_read,
						shared_from_this(),
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));
			}
			else
				close();
		}

	private:

		void handle_downstream_write(const boost::system::error_code& error)
		{
			if (!error)
			{
				upstream_.async_read_some(
					boost::asio::buffer(upstream_buffer_, buffer_size),
					boost::bind(&tunnel::handle_upstream_read,
						shared_from_this(),
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));
			}
			else
				close();
		}

		void handle_downstream_read(const boost::system::error_code& error,
			const size_t& bytes_transferred)
		{
			if (!error)
			{
				async_write(upstream_,
					boost::asio::buffer(downstream_buffer_, bytes_transferred),
					boost::bind(&tunnel::handle_upstream_write,
						shared_from_this(),
						boost::asio::placeholders::error));
			}
			else
				close();
		}

		void handle_upstream_write(const boost::system::error_code& error)
		{
			if (!error)
			{
				downstream_.async_read_some(
					boost::asio::buffer(downstream_buffer_, buffer_size),
					boost::bind(&tunnel::handle_downstream_read,
						shared_from_this(),
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));
			}
			else
				close();
		}

		void handle_upstream_read(const boost::system::error_code& error,
			const size_t& bytes_transferred)
		{
			if (!error)
			{
				async_write(downstream_,
					boost::asio::buffer(upstream_buffer_, bytes_transferred),
					boost::bind(&tunnel::handle_downstream_write,
						shared_from_this(),
						boost::asio::placeholders::error));
			}
			else
				close();
		}

		void close()
		{
			boost::mutex::scoped_lock lock(mutex_);

			if (downstream_.is_open())
			{
				downstream_.close();
			}

			if (upstream_.is_open())
			{
				upstream_.close();
			}
		}

		socket_type downstream_;
		socket_type upstream_;

		enum { buffer_size = 8192 };
		unsigned char downstream_buffer_[buffer_size];
		unsigned char upstream_buffer_[buffer_size];

		boost::mutex mutex_;

	public:		
		class tunnel_host
		{
		public:			
			tunnel_host(boost::asio::io_service& io_service,
				const std::string& local_host, unsigned short local_port,
				boost::function<ip_node_type()> next_upstream)
				: io_service_(io_service),
				localhost_address(boost::asio::ip::address_v4::from_string(local_host)),
				tcp_acceptor_(io_service_, ip::tcp::endpoint(localhost_address, local_port)),
				next_upstream_(next_upstream)
			{}

			bool run()
			{
				try
				{
					tunnel_ = boost::make_shared<tunnel>(io_service_);

					tcp_acceptor_.async_accept(tunnel_->downstream_socket(),
						boost::bind(&tunnel_host::handle_accept,
							this,
							boost::asio::placeholders::error));
				}
				catch (std::exception& e)
				{
					std::cerr << "accept exception: " << e.what() << std::endl;
					return false;
				}

				return true;
			}

		private:

			void handle_accept(const boost::system::error_code& error)
			{
				if (!error)
				{
					auto next_node = next_upstream_();

					tunnel_->start(next_node.ip(), next_node.port());

					if (!run())
					{
						std::cerr << "accept failed." << std::endl;
					}
				}
				else
				{
					std::cerr << "Error: " << error.message() << std::endl;
				}
			}

			boost::asio::io_service& io_service_;
			ip::address_v4 localhost_address;
			ip::tcp::acceptor tcp_acceptor_;
			ptr_type tunnel_;
			boost::function<ip_node_type()> next_upstream_;
		};
	};
}
