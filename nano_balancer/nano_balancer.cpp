//          Copyright Michael Shmalko 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <iostream>
#include "tunnel_host.hpp"
#include "probe.hpp"
#include "helper.hpp"
#include <boost/process.hpp>
#include <windows.h>
#include "mdump.h"
using namespace nano_balancer;


int main(int argc, char* argv[])
{
	MiniDumper dumper("nano_balancer");
	if (argc != 2 && argc < 4)
	{
		std::cerr << "usage: nano_balancer <master_config>\r\n\t nano_balancer <local host ip> <local port> <config>" << std::endl;
		return 1;
	}

	const auto is_child = argc > 3;
	const std::string config_file = argv[is_child ? 3 : 1];
	try
	{		
		if (is_child)
		{
			// run as load balancer
			const auto local_port = static_cast<unsigned short>(::atoi(argv[2]));
			const std::string local_host = argv[1];
			
			boost::asio::io_service ios;

			auto probe = boost::make_shared<nano_balancer::probe>(ios, config_file);
			probe->start();

			tunnel::tunnel_host acceptor(
				ios,
				local_host,
				local_port,
				boost::bind(&probe::get_next_node, probe->shared_from_this())
			);

			acceptor.run();

			ios.run();
		}
		else
		{
			// run as master host process
			auto instances = helper::parse_master_config(config_file);
			std::list<std::shared_ptr<boost::process::child>> children;
			for (auto instance : instances)
			{
				boost::char_separator<char> sep(" ");
				boost::tokenizer<boost::char_separator<char>> tok(instance, sep);
				std::vector<std::string> cmd_line;
				for (auto cs : tok)
				{
					cmd_line.push_back(cs);
				}

				children.push_back(std::make_shared<boost::process::child>("nano_balancer.exe", cmd_line));
			}
			for (auto child : children)
			{
				child->wait();
			}
		}
	}
	catch (std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}