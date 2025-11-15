#include "../includes/configHandler.hpp"
#include "../includes/configUtils.hpp"
#include <iostream>

serverConfig configHandler::host;

void configHandler::fillPorts(t_host &newHost, std::stack<std::string> &blockTokens)
{
	std::string token = blockTokens.top();
	blockTokens.pop();
	try
	{
		while (token != "]")
		{
			configUtils::checkPort(token);
			newHost.ports.push_back(token);
			blockTokens.pop();
			token = blockTokens.top();
		}
	}
	catch (const std::exception &e)
	{
		throw errorHandler(std::string(e.what()));
	}
	blockTokens.pop();
}

void configHandler::setMaxReqBody(t_host &newHost, std::string const &prop)
{
	unsigned int	maxBody;

	try
	{
		maxBody = configUtils::toNum(prop);
		if (maxBody > 1048576)
			throw errorHandler(INVALID_INSTRUCTION, prop);
	}
	catch (const std::exception &e)
	{
		throw errorHandler(std::string(e.what()));
	}
	newHost.maxReqBody = maxBody;
}

void configHandler::setTimeout(t_host &newHost, std::string const &prop)
{
	unsigned int	timeout;

	try
	{
		timeout = configUtils::toNum(prop);
		if (timeout > 3600)
			throw errorHandler(INVALID_INSTRUCTION, prop);
	}
	catch (const std::exception &e)
	{
		throw errorHandler(std::string(e.what()));
	}
	newHost.hostTimeout = timeout;
}

void configHandler::fillHostConf(std::stack<std::string> &blockTokens)
{
	t_host			newHost;
	unsigned short	count;

	std::string propName, prop;
	count = 0;
	try
	{
		while (!blockTokens.empty())
		{
			if (!configUtils::getFromStack(propName, blockTokens)
				|| !configUtils::getFromStack(prop, blockTokens))
				throw errorHandler(MISSING_PROPERTY, "in host_config");
			if (propName == "addr")
			{
				configUtils::checkAddr(prop);
				newHost.addr = prop;
				count++;
			}
			else if (propName == "ports")
			{
				fillPorts(newHost, blockTokens);
				count++;
			}
			else if (propName == "default_page")
			{
				newHost.page = prop;
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
			else if (propName == "default_root")
			{
				configUtils::ifDir(prop);
				newHost.root = prop;
				count++;
			}
			else
				throw errorHandler(INVALID_INSTRUCTION, propName);
		}
		if (count < 6)
			throw errorHandler(MISSING_PROPERTY, " host config");
		configUtils::ifPage(newHost.root, newHost.page);
	}
	catch (const std::exception &e)
	{
		throw errorHandler(std::string(e.what()));
	}
	host.setHost(newHost);
}

void configHandler::fillErrPg(std::stack<std::string> &blockTokens)
{
	unsigned int	error;

	std::string propName, prop;
	try
	{
		while (!blockTokens.empty())
		{
			if (!configUtils::getFromStack(propName, blockTokens)
				|| !configUtils::getFromStack(prop, blockTokens))
				throw errorHandler(MISSING_PROPERTY, "in route");
			error = configUtils::toNum(propName);
			host.addErrorPages(error, prop);
		}
	}
	catch (const std::exception &e)
	{
		throw errorHandler(std::string(e.what()));
	}
}

void configHandler::fillRoute(std::stack<std::string> &blockTokens)
{
	t_route			route;
	unsigned short	count;
	unsigned int	resp;

	std::string key = blockTokens.top();
	std::string propName, prop;
	count = 0;
	blockTokens.pop();
	if (key.at(key.size() - 1) == ':')
		host.addRoute("", route);
	try
	{
		while (!blockTokens.empty())
		{
			if (!configUtils::getFromStack(propName, blockTokens)
				|| !configUtils::getFromStack(prop, blockTokens))
				throw errorHandler(MISSING_PROPERTY, "in route");
			if (propName == "new_root")
			{
				configUtils::ifDir(prop);
				route.newRoot = prop;
				count++;
			}
			else if (propName == "page")
			{
				route.page = prop;
				count++;
			}
			else if (propName == "success_response")
			{
				resp = configUtils::toNum(prop);
				if (resp < 100 || resp > 599)
					throw errorHandler(INVALID_INSTRUCTION, prop);
				route.response = resp;
				count++;
			}
		}
		if (count < 3)
			throw errorHandler(MISSING_PROPERTY, " route");
		configUtils::ifPage(route.newRoot, route.page);
	}
	catch (const std::exception &e)
	{
		throw errorHandler(std::string(e.what()));
	}
	host.addRoute(key, route);
}

void configHandler::fillLoc(std::stack<std::string> &blockTokens)
{
	t_location		loc;
	unsigned short	count;

	std::string key = blockTokens.top();
	std::string propName, prop;
	count = 0;
	blockTokens.pop();
	if (key.at(key.size() - 1) == ':')
		host.addLocation("", loc);
	try
	{
		while (!blockTokens.empty())
		{
			if (!configUtils::getFromStack(propName, blockTokens)
				|| !configUtils::getFromStack(prop, blockTokens))
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
	catch (const std::exception &e)
	{
		throw errorHandler(std::string(e.what()));
	}
	if (count < 3)
		throw errorHandler(MISSING_PROPERTY, " location");
	host.addLocation(key, loc);
}

void configHandler::fillCgiConf(std::stack<std::string> &blockTokens)
{
	t_cgi			cgi;
	unsigned short	count;

	std::string propName, prop;
	count = 0;
	while (!blockTokens.empty())
	{
		if (!configUtils::getFromStack(propName, blockTokens)
			|| !configUtils::getFromStack(prop, blockTokens))
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

serverConfig configHandler::getHost(void)
{
	return (host);
}
