#include "../includes/configHandler.hpp"
#include "../includes/configUtils.hpp"
#include <iostream>

serverConfig configHandler::host;

void configHandler::fillAddr(t_host &newHost, std::string const &line)
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
	unsigned int maxBody;

	try
	{
		maxBody = configUtils::toNum(prop);
		if (maxBody > 1048576)
			throw errorHandler(INVALID_INSTRUCTION, prop);
	}
	catch(const std::exception& e)
	{
		throw errorHandler(std::string(e.what()));
	}
	newHost.maxReqBody = maxBody;
}

void configHandler::setTimeout(t_host &newHost, std::string const &prop)
{
	unsigned int timeout;
	try
	{
		timeout = configUtils::toNum(prop);
		if (timeout > 3600)
			throw errorHandler(INVALID_INSTRUCTION, prop);
	}
	catch(const std::exception& e)
	{
		throw errorHandler(std::string(e.what()));
	}
	newHost.hostTimeout = timeout;
}

void configHandler::fillHostConf(std::stack<std::string> &blockTokens)
{
	t_host	newHost;
	std::string prop;
	std::string propName;
	unsigned short count = 0;
	
	try
	{
		while (!blockTokens.empty())
		{
			if (!configUtils::getFromStack(propName, blockTokens) || !configUtils::getFromStack(prop, blockTokens))
				throw errorHandler(MISSING_PROPERTY, "in route");
			if (propName == "host_name")
			{
				newHost.hostName = prop;
				count++;
			}
			else if (propName == "listen")
			{
				fillAddr(newHost, prop);
				count++;
			}
			else if (propName == "default_page")
			{
				configUtils::ifFile(prop);
				newHost.defaulPage = prop;
				count++;
			}
			else if (propName == "max_request_body")
			{
				setMaxReqBody(newHost, prop);
				count++;
			}
			else if (propName == "host_timeout")
			{
				setTimeout(newHost, prop);
				count++;
			}
			else
				throw errorHandler(INVALID_INSTRUCTION, propName);
		}
	}
	catch (const std::exception &e)
	{
		throw errorHandler(std::string(e.what()));
	}
	if (count < 5)
		throw errorHandler(MISSING_PROPERTY, " host config");
	host.setHost(newHost);
}

void configHandler::fillErrPg(std::stack<std::string> &blockTokens)
{
	std::string propName;
	std::string prop;

	try
	{
		while (!blockTokens.empty())
		{
			if (!configUtils::getFromStack(propName, blockTokens) || !configUtils::getFromStack(prop, blockTokens))
				throw errorHandler(MISSING_PROPERTY, "in route");
			unsigned int error = configUtils::toNum(propName);
			host.addErrorPages(error, prop);
		}
	}
	catch(const std::exception& e)
	{
		throw errorHandler(std::string(e.what()));
	}
}

void configHandler::fillRoute(std::stack<std::string> &blockTokens)
{
	std::string key = blockTokens.top();
	t_route route;
	std::string propName;
	std::string prop;
	unsigned short count = 0;

	blockTokens.pop();
	if (key.at(key.size() - 1) == ':')
		host.addRoute("", route);
	try
	{
		while (!blockTokens.empty())
		{
			if (!configUtils::getFromStack(propName, blockTokens) || !configUtils::getFromStack(prop, blockTokens))
				throw errorHandler(MISSING_PROPERTY, "in route");
			if (propName == "new_root")
			{
				configUtils::ifDir(prop);
				route.newRoot = prop;
				count++;
			}
			else if (propName == "page")
			{
				if (prop == "none")
					route.page = prop;
				else
				{
					std::string filePath = route.newRoot;
					configUtils::concatFilePath(filePath, prop);
					configUtils::ifFile(filePath);
					route.page = prop;
				}
				count++;
			}
			else if (propName == "success_response")
			{
				unsigned int resp = configUtils::toNum(prop);
				if (resp < 100 || resp > 599)
					throw errorHandler(INVALID_INSTRUCTION, prop);
				route.response = resp;
				count++;
			}
		}
	}
	catch(const std::exception& e)
	{
		throw errorHandler(std::string(e.what()));
	}
	if (count < 3)
		throw errorHandler(MISSING_PROPERTY, " route");
	host.addRoute(key, route);
}

void configHandler::fillLoc(std::stack<std::string> &blockTokens)
{
	std::string key = blockTokens.top();
	t_location loc;
	std::string propName;
	std::string prop;
	unsigned short count = 0;

	blockTokens.pop();
	if (key.at(key.size() - 1) == ':')
		host.addLocation("", loc);
	try
	{
		while (!blockTokens.empty())
		{
			if (!configUtils::getFromStack(propName, blockTokens) || !configUtils::getFromStack(prop, blockTokens))
				throw errorHandler(MISSING_PROPERTY, "in route");
			if (propName == "directory_listing")
			{
				loc.enableListing = configUtils::onOff(prop);
				count++;
			}
			else if (propName == "upload_enabled")
			{
				loc.enableUpload = configUtils::onOff(prop);
				count++;
			}
			else if (propName == "methods")
			{
				configUtils::getFromList(loc, blockTokens);
				count++;
			}
		}
	}
	catch(const std::exception& e)
	{
		throw errorHandler(std::string(e.what()));
	}
	if (count < 3)
		throw errorHandler(MISSING_PROPERTY, " location");
	host.addLocation(key, loc);
}

void configHandler::fillCgiConf(std::stack<std::string> &blockTokens)
{
	t_cgi cgi;
	std::string propName;
	std::string prop;
	unsigned short count = 0;

	while (!blockTokens.empty())
	{
		if (!configUtils::getFromStack(propName, blockTokens) || !configUtils::getFromStack(prop, blockTokens))
				throw errorHandler(MISSING_PROPERTY, "in route");
		if (propName == "cgi_allowed")
		{
			cgi.cgiAllowed = configUtils::onOff(prop);
			count++;
		}
		else if (propName == "cgi_extensions")
		{
			configUtils::getFromList(cgi, blockTokens);
			count++;
		}
		else if (propName == "root")
		{
			configUtils::ifDir(prop);
			cgi.root = prop;
			count++;
		}
		else
			throw errorHandler(INVALID_INSTRUCTION, prop);
	}
	if (count < 3)
		throw errorHandler(MISSING_PROPERTY, " location");
}

serverConfig configHandler::getHost(void) {return host;}
