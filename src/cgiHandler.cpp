#include "../includes/cgiHandler.hpp"

cgiHandler::cgiHandler(serverConfig const &conf, t_request const &req) : config(conf), request(req), success(false) {}

cgiHandler::~cgiHandler(void) {}

void cgiHandler::setEnv(void)
{
	std::map<std::string, std::string>::iterator it = request.headers.begin();
	std::map<std::string, std::string>envVars = config.getEnv();
	for (; it != request.headers.end(); ++it)
	{
		std::map<std::string, std::string>::iterator found = envVars.find(it->first);
		if (found != envVars.end())
		{
			std::string var = found->second + "=" + it->second;
			env.push_back(var.c_str());
		}
	}
}
