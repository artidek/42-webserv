#include <iostream>
#include "includes/configParser.hpp"
#include "includes/server.hpp"

int main (int argc, char **argv)
{
	(void)argv;
	if (argc > 2)
	{
		std::cout << "Error: Wrong number of arguments => usage ./webserv | ./webserv [.conf]\n";
		return 1;
	}
	if (argc == 1)
	{
		try
		{
			configParser::parseConfig("default.conf");
			server srv(configParser::getConfigs());
			srv.set();
			srv.run();
		}
		catch(const std::exception& e)
		{
			std::cerr << e.what() << '\n';
			return 1;
		}
	}
	else
	{
		try
		{
			configParser::parseConfig(argv[1]);
			server srv(configParser::getConfigs());
			srv.set();
			srv.run();
		}
		catch(const std::exception& e)
		{
			std::cerr << e.what() << '\n';
			return 1;
		}
	}
}