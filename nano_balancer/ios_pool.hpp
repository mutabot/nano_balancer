#pragma once
#include <deque>

#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <boost/asio/io_service.hpp>

namespace nano_balancer
{
	class ios_thread_pool
	{
	public:

		typedef boost::shared_ptr<boost::thread> thread_t;

		ios_thread_pool()
			: thread_count_(0)
		{}

		ios_thread_pool& set_thread_count(const std::size_t thread_count)
		{
			thread_count_ = thread_count;
			return *this;
		}

		void run(boost::asio::io_service& ios)
		{
			for (std::size_t i = 0; i < thread_count_; ++i)
			{
				thread_t thread(new boost::thread(boost::bind(&boost::asio::io_service::run, &ios)));
				thread_pool_.push_back(thread);
			}

			for (std::size_t i = 0; i < thread_pool_.size(); ++i)
			{
				thread_pool_[i]->join();
			}
		}

	private:

		std::size_t thread_count_;
		std::deque<thread_t> thread_pool_;
	};
}