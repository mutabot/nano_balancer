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
#include "logging.h"

using namespace nano_balancer;

int main(int argc, char* argv[])
{
	logging::core::get()->set_filter
	(
		logging::trivial::severity >= logging::trivial::debug
	);
	logging::add_common_attributes();

	
	logger_type lg;

	MiniDumper dumper("nano_balancer");

	time_stamp_stream stdout_time(std::cout);
	time_stamp_stream stderr_time(std::cerr);

	if (argc != 2 && argc < 4)
	{
		std::cerr << "usage: nano_balancer <master_config>\r\n\t nano_balancer <local host ip> <local port> <config>";
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

			std::ostringstream child_log_name;
			child_log_name << "..\\log\\nano_child_" << local_host << local_port << "_%Y%m%d_%H%M%S.%3N.log";

			add_log_file(child_log_name.str());

			BOOST_LOG_SEV(lg, trivial::info) << "Running as child on: " << local_host << ":" << local_port;
			
			boost::asio::io_service ios;

			auto probe = boost::make_shared<nano_balancer::probe>(lg, ios, config_file);
			probe->start();

			tunnel::tunnel_host acceptor(
				lg,
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
			add_log_file("..\\log\\nano_balancer_%Y%m%d_%H%M%S.%3N.log");

			BOOST_LOG_SEV(lg, trivial::info) << "Running as master: " << config_file;
			// run as master host process
			auto instances = helper::parse_master_config(config_file);
			auto host = process_host(lg, instances);
			host.run();
		}
	}
	catch (std::exception& e)
	{
		BOOST_LOG_SEV(lg, trivial::error) << "Error: " << e.what();
		return 1;
	}

	return 0;
}
