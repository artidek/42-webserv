#include <iostream>
#include "includes/configParser.hpp"

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
		}
		catch(const std::exception& e)
		{
			std::cerr << e.what() << '\n';
			return 1;
		}
	}
}