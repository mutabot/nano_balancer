//          Copyright Michael Shmalko 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include "tunnel_host.hpp"
#include "probe.hpp"
#include "helper.hpp"
using namespace nano_balancer;

int main(int argc, char* argv[])
{
	if (argc != 4)
	{
		std::cerr << "usage: nano_balancer <local host ip> <local port> <forward node list file>" << std::endl;
		return 1;
	}

	const unsigned short local_port = static_cast<unsigned short>(::atoi(argv[2]));
	const std::string local_host = argv[1];
	const std::string forward_host_file = argv[3];
	
	boost::asio::io_service ios;
	nano_balancer::probe::ptr_type probe;
	try
	{
		probe = boost::make_shared<nano_balancer::probe>(ios, forward_host_file);
		probe->start();

		nano_balancer::tunnel::tunnel_host acceptor(
			ios,
			local_host,
			local_port,
			boost::bind(&probe::get_next_node, probe->shared_from_this())
		);

		acceptor.run();

		ios.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}

