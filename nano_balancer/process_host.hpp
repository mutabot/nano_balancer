//          Copyright Michael Shmalko 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <map>
#include <boost/process/child.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/token_functions.hpp>
#include <boost/tokenizer.hpp>
#include "logging.h"
#include <boost/thread/thread.hpp>

namespace nano_balancer
{
	class process_host 
	{
		typedef std::vector<std::string> cmd_line_type;

		std::list<std::string> instances_;

		std::list<std::shared_ptr<boost::process::child>> children;
		std::map<boost::process::pid_t, cmd_line_type> child_l_map;
		logger_type& logger_;

		bool start_new_child(cmd_line_type cmd_line)
		{
			auto cmd_str = boost::algorithm::join(cmd_line, " ");
			
			BOOST_LOG_SEV(logger_, trivial::info) << "Starting new: " << cmd_str;
			auto new_child = std::make_shared<boost::process::child>("nano_balancer.exe", cmd_line);
			
			// check if child is failing immediately
			if (new_child->wait_until(std::chrono::system_clock::time_point(std::chrono::system_clock::now() + std::chrono::seconds(2))))
			{
				BOOST_LOG_SEV(logger_, trivial::error) << "Error: Failed to start new child: " << new_child->id();
				return false;				
			}

			BOOST_LOG_SEV(logger_, trivial::info) << "New child PID: " << new_child->id();
			child_l_map.insert_or_assign(new_child->id(), cmd_line);
			children.push_back(new_child);
			return true;			
		}
	public:
		process_host(logger_type& logger, std::list<std::string>& instances)
		: instances_(instances), logger_(logger)
	{
		}

		void run()
		{
			for (auto instance : instances_)
			{
				boost::char_separator<char> sep(" ");
				boost::tokenizer<boost::char_separator<char>> tok(instance, sep);
				cmd_line_type cmd_line;
				for (auto cs : tok)
				{
					cmd_line.push_back(cs);
				}

				start_new_child(cmd_line);

				// boost::this_thread::sleep(boost::posix_time::seconds(1));
			}

			while (true)
			{
				for (auto child : children)
				{
					if (child->wait_until(std::chrono::system_clock::time_point(std::chrono::system_clock::now() + std::chrono::seconds(2))))
					{
						BOOST_LOG_SEV(logger_, trivial::error) << "Error: Child is not running: " << child->id();

						start_new_child(child_l_map[child->id()]);

						child_l_map.erase(child->id());
						children.remove(child);

						break;
					}
				}
			}
		}

		~process_host()
		{
			
		};
	};
}
