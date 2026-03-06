#include <iostream>
#include "includes/configParser.hpp"
#include "includes/server.hpp"
#include <csignal>

int main (int argc, char **argv, char **env)
{
	(void)argv;
	if (argc > 2)
	{
		std::cout << "Error: Wrong number of arguments => usage ./webserv | ./webserv [.conf]\n";
		return 1;
	}
	configParser parser;
	if (argc == 1)
	{
		std::signal(SIGINT, server::handle_signal);
		try
		{
			parser.parseConfig("default.conf");
			server srv(parser.getConfigs(), env);
			srv.set(parser.getTimeout());
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
			parser.parseConfig(argv[1]);
			server srv(parser.getConfigs(), env);
			srv.set(parser.getTimeout());
			srv.run();
		}
		catch(const std::exception& e)
		{
			std::cerr << e.what() << '\n';
			return 1;
		}
	}
}