//          Copyright Michael Shmalko 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <list>
#include "types.h"
#include <fstream>
#include <regex>

namespace nano_balancer
{
	class helper
	{
	public:
		static std::list<std::string> parse_master_config(const std::string& config_file_name)
		{
			std::list<std::string> result;
			std::ifstream file(config_file_name);
			std::string line;
			while (std::getline(file, line))
			{
				result.push_back(line);
			}
			return result;
		}

		static std::list<ip_node_type> parse_config(const std::string& config_file_name)
		{
			std::list<ip_node_type> result;
			std::ifstream file(config_file_name);
			std::string line;
			while (std::getline(file, line))
			{
				try {
					std::regex re("^((?:(?:[0-9]{1,3}\\.){3})[0-9]{1,3}):([0-9]{1,5})$");
					std::smatch match;
					if (std::regex_search(line, match, re) && match.size() > 2)
					{
						auto address_str = match.str(1);
						auto port_str = match.str(2);
						ip_node_type n(boost::asio::ip::address_v4::from_string(address_str), std::stoi(port_str));
						result.push_back(n);
					}
					else
					{
						std::cerr << "Config line skipped: " << line << std::endl;
					}
				}
				catch (std::regex_error& e)
				{
					std::cerr << "Error: " << e.what() << std::endl;
				}
			}
			return result;
		}
	};
};