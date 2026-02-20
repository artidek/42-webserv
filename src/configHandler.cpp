#include "../includes/configHandler.hpp"
#include "../includes/configUtils.hpp"
#include <iostream>

serverConfig configHandler::host;

void configHandler::fillPorts(t_host &newHost, std::stack<std::string> &blockTokens)
{
	std::string token = blockTokens.top();
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
			else if (propName == "hostname")
			{
				if (prop.empty())
					throw errorHandler(CONFIG_EMPTY, "hostname in host config");
				newHost.hostname = prop;
				count++;
			}
			else
				throw errorHandler(INVALID_INSTRUCTION, propName);
		}
		if (count < 5)
			throw errorHandler(MISSING_PROPERTY, " host config");
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
	if (key.at(0) != '/' || key.empty())
		throw errorHandler(MISSING_TOKEN, "/ in route name");

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
				if (prop[prop.size() - 1] != '/')
					prop += "/";
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
				if (!configUtils::isResCode(resp))
					throw errorHandler(INVALID_INSTRUCTION, prop);
				route.response = prop;
				count++;
			}
			else if (propName == "methods")
			{
				if (prop == "none")
					route.methods.push_back(prop);
				else
					configUtils::getFromList(route, blockTokens);
				count++;
			}
			else if (propName == "redirect")
			{
				if (prop.empty() || (prop[0] != '/' && prop != "none"))
					throw errorHandler(MISSING_TOKEN, "/ in route redirect");
				route.redirect = prop;
				count++;
			}
		}
		if (count < 5)
			throw errorHandler(MISSING_PROPERTY, " in route");
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
			else if (propName == "list_ext")
			{
				if (prop == "none")
					loc.listExt.push_back(prop);
				else
					configUtils::getFromList(loc.listExt, blockTokens);
				count++;
			}
			else if (propName == "upload_ext")
			{
				if (prop == "none")
					loc.uploadExt.push_back(prop);
				else
					configUtils::getFromList(loc.uploadExt, blockTokens);
				count++;
			}
		}
	}
	catch (const std::exception &e)
	{
		throw errorHandler(std::string(e.what()));
	}
	if (count < 4)
		throw errorHandler(MISSING_PROPERTY, " location");
	if (key[key.size() - 1] != '/')
		key += "/";
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
			throw errorHandler(MISSING_PROPERTY, " cgi config");
		if (propName == "cgi_allowed")
		{
			cgi.cgiAllowed = configUtils::onOff(prop);
			count++;
		}
		else if (propName == "root")
		{
			configUtils::ifDir(prop);
			cgi.root = prop;
			count++;
		}
		else if (propName == "default_cgi")
		{
			cgi.defaultCgi = prop;
			count++;
		}
		else if (propName == "cgi_extensions")
		{
			configUtils::getFromList(cgi, blockTokens);
			count++;
		}
		else
			throw errorHandler(INVALID_INSTRUCTION, prop);
	}
	if (count < 4)
		throw errorHandler(MISSING_PROPERTY, " cgi config");
}

serverConfig configHandler::getHost(void)
{
	return (host);
}
