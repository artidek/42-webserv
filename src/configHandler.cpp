#include "../includes/configHandler.hpp"
#include "../includes/configUtils.hpp"

static serverConfig host;

bool configHandler::fillAddr(t_host &newHost, std::string const &line)
{
	std::stringstream ss(line);
	std::string addr;
	std::string port;
	if (std::getline(ss, addr, ':') && std::getline(ss, port))
	{
		if (!configUtils::checkAddr(addr))
			throw errorHandler(INVALID_INSTRUCTION, addr);
		if (!configUtils::checkPort(port))
			throw errorHandler(INVALID_INSTRUCTION, port);
	}
	else
		throw errorHandler(INVALID_INSTRUCTION, line);
	newHost.addr = addr;
	newHost.port = port;
}

void configHandler::setMaxReqBody(t_host &newHost, std::string const &prop)
{
	std::stringstream ss(prop);
	unsigned int maxBody;

	ss >> maxBody;
	if (ss.fail() || maxBody > 1048576)
		throw errorHandler(INVALID_INSTRUCTION, prop);
}

void configHandler::setTimeout(t_host &newHost, std::string const &prop)
{
	std::stringstream ss(prop);
	unsigned int timeout;

	ss >> timeout;
	if (ss.fail() || timeout > 3600)
		throw errorHandler(INVALID_INSTRUCTION, prop);
}

void configHandler::fillHostConf(std::stack<std::string> &blockTokens)
{
	t_host	newHost;

	std::string prop;
	std::string propName;
	std::string token;
	try
	{
		while (!blockTokens.empty())
		{
			token = blockTokens.top();
			propName = token.substr(0, token.size() - 1);
			blockTokens.pop();
			token = blockTokens.top();
			prop = token.substr(0, token.size() - 1);
			blockTokens.pop();
			if (propName == "host_name")
				newHost.hostName = prop;
			else if (propName == "listen")
				fillAddr(newHost, prop);
			else if (propName == "default_page")
			{
				configUtils::ifFile(prop);
				newHost.defaulPage = prop;
			}
			else if (propName == "max_request_body")
				setMaxReqBody(newHost, prop);
			else if (propName == "host_timeout")
				setTimeout(newHost, prop);
		}
	}
	catch (const std::exception &e)
	{
		throw errorHandler(std::string(e.what()));
	}
	host.setHost(newHost);
}

void configHandler::fillErrPg(std::stack<std::string> &blockTokens)
{
	while (!blockTokens.empty())
		blockTokens.pop();
}

void configHandler::fillRoute(std::stack<std::string> &blockTokens)
{
	while (!blockTokens.empty())
		blockTokens.pop();
}

void configHandler::fillLoc(std::stack<std::string> &blockTokens)
{
	while (!blockTokens.empty())
		blockTokens.pop();
}

void configHandler::fillCgiConf(std::stack<std::string> &blockTokens)
{
	while (!blockTokens.empty())
	{
		blockTokens.pop();
	}
}

serverConfig configHandler::getHost(void) {return host;}
