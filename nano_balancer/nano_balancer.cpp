//          Copyright Michael Shmalko 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <iostream>
#include "tunnel_host.hpp"
#include "process_host.hpp"
#include "probe.hpp"
#include "helper.hpp"
#include <windows.h>
#include "mdump.h"
#include "time_stamp_stream.hpp"

using namespace nano_balancer;

int main(int argc, char* argv[])
{
	MiniDumper dumper("nano_balancer");

	time_stamp_stream stdout_time(std::cout);
	time_stamp_stream stderr_time(std::cerr);

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
			auto host = process_host(instances);
			host.run();
		}
	}
	catch (std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
