#include "../includes/configParser.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include "../includes/configUtils.hpp"
#include "../includes/configHandler.hpp"


configParser::configParser() : blockEnd(false), timeout(0) {}

configParser::~configParser() {}

void configParser::flatten(std::ifstream const &file)
{
	std::ostringstream buffer;
	buffer << file.rdbuf();
	if (buffer.str().empty())
		throw errorHandler(CONFIG_EMPTY);
	flattened = buffer.str();
	flattened.erase(std::remove(flattened.begin(), flattened.end(), '\n'),
		flattened.end());
}

void configParser::listToken(size_t &i, std::string &token)
{
	std::string cleanedList;
	size_t found = conf.find(']', i);
	if (found == std::string::npos)
		throw errorHandler(CONFIG_EMPTY, "list");
	std::string list = conf.substr(i + 1, found - (i + 1));
	list = configUtils::trim(list, " \t");
	for (size_t j = 0; j < list.size(); j++)
	{
		if (!isspace(static_cast<unsigned char>(list[j])) && list[j] != '[' && list[j] != ']')
			cleanedList += list[j];
		else if (list[j] == '[')
			throw errorHandler(MISSING_TOKEN, "]");
		else
		{
			if (cleanedList.empty())
				throw errorHandler(WRONG_TOKEN, "[ or, ]");
			if (!cleanedList.empty() && cleanedList[cleanedList.size() - 1] != ',')
				throw errorHandler(MISSING_TOKEN, ", or ]");
		}
	}
	cleanedList = "[" + cleanedList + "]";
	token += cleanedList;
	tokens.push(token);
	token.clear();
	i = found;
}

void configParser::tokenize(void)
{
	std::string server;
	std::stringstream ss(flattened);
	bool host = false;
	if (std::getline(ss, server, '{'))
	{
		server = configUtils::trim(server, " \t");
		if (server != "server")
			throw errorHandler(INVALID_INSTRUCTION, server);
	}
	else
		throw errorHandler(INVALID_INSTRUCTION, server);
	size_t startConf = flattened.find("{");
	if (startConf == std::string::npos)
		throw errorHandler(MISSING_TOKEN, "{");
	size_t endConf = flattened.find_last_of("}");
	if (endConf == std::string::npos)
		throw errorHandler(MISSING_TOKEN, "}");
	conf = flattened.substr(startConf + 1, endConf - (startConf + 1));
	try
	{
		std::string token;
		for (size_t i = 0; i < conf.size(); i++)
		{
			if (conf[i] == '{')
			{
				if (token.empty())
					throw errorHandler(MISSING_PROPERTY, "start of block");
				token = configUtils::trim(token, " \t");
				std::stringstream ss(token);
				std::string blockName;
				std::string blockProperty;
				if (std::getline(ss,blockName, ' '))
				{
					if (blockName == "host" && !host)
					{
						host = true;
						tokens.push(token);
						token.clear();
						continue;
					}
					if (!checkBlocknames(blockName))
						throw errorHandler(INVALID_INSTRUCTION, blockName);
				}
				tokens.push(token);
				token.clear();
				i += 1;
				extractBlockProp(i);
			}
			else if (conf[i] == ';')
			{
				if (token.empty())
					throw errorHandler(MISSING_PROPERTY, "empty property not allowed");
				if (token.find("request_timeout:") == std::string::npos)
					throw errorHandler(INVALID_INSTRUCTION, token);
				else
				{
					std::stringstream ss(token);
					std::string name;
					std::string val;
					if (std::getline(ss, name, ' ') && std::getline(ss, val))
					{
						val = configUtils::trim(val, " \t");
						if (val.empty())
							throw errorHandler(MISSING_PROPERTY, token);
						if (timeout > 0)
							throw errorHandler (INVALID_INSTRUCTION, "duplicate not allowed " + token);
						timeout = configUtils::toNum(val);
					}
					else
						throw errorHandler(INVALID_INSTRUCTION, token);
				}
				token.clear();
			}
			else if (conf[i] == '}')
			{
				if (host)
				{
					host = false;
					continue;
				}
				else
					throw errorHandler(WRONG_TOKEN, "}");
			}
			else
				token += conf[i];
		}
		if (host)
			throw errorHandler(INVALID_INSTRUCTION, "no } in host");
		if (!token.empty())
			throw errorHandler(INVALID_INSTRUCTION, token);
	}
	catch(const std::exception& e)
	{
		throw errorHandler(std::string(e.what()));
	}
}

void configParser::checkBlock(std::stack<std::string> &blockTokens)
{
	std::string name, val;
	std::stringstream ss(tokens.top());
	if (std::getline(ss, name, ' '))
	{
		if (std::getline(ss, val))
		{
			val = configUtils::trim(val, " \t");
			blockTokens.push(val);
		}
	}
	else
		throw errorHandler(INVALID_INSTRUCTION, tokens.top());
	blockNames[name](blockTokens);
	tokens.pop();
}

void configParser::parseList(std::stack<std::string> &blockTokens)
{
	std::string token = tokens.top();
	std::string name = token.substr(0, token.find(":"));
	size_t startList = token.find("[");
	size_t endList = token.find("]");
	token = token.substr(startList + 1, endList - (startList + 1));
	std::stringstream ss(token);
	tokens.pop();
	std::string val;
	blockTokens.push("end_list");
	while (std::getline(ss,val, ',') || std::getline(ss, val))
		blockTokens.push(val);
	blockTokens.push(name);
}

 void configParser::parseBlock(void)
{
	std::stack<std::string> blockTokens;
	try
 	{
		std::string token;
		while(!tokens.empty())
		{
			token = tokens.top();
			if (!token.empty() && token[token.size() -1] == ';')
			{
				std::stringstream ss(token);
				std::string name, val;
				if (std::getline(ss, name, ':') && std::getline(ss, val, ';'))
				{
					blockTokens.push(val);
					blockTokens.push(name);
				}
				else
					throw errorHandler(INVALID_INSTRUCTION, token);
				token.clear();
				tokens.pop();
			}
			else if(!token.empty() && token[token.size() -1] == ']')
				parseList(blockTokens);
			else
			{
				if (token == "host")
					addConfig();
				else
					checkBlock(blockTokens);
			}
		}
 	}
 	catch (const std::exception &e)
 	{
 		throw errorHandler(std::string(e.what()));
 	}
 }

void configParser::initBlockNames(void)
{
	blockNames["host_configs"] = configHandler::fillHostConf;
	blockNames["error_pages"] = configHandler::fillErrPg;
	blockNames["route"] = configHandler::fillRoute;
	blockNames["cgi_config"] = configHandler::fillCgiConf;
	blockNames["location"] = configHandler::fillLoc;
}

void configParser::parseConfig(std::string confFile)
{
	try
	{
		configUtils::checkExt(confFile);
		std::ifstream file(confFile.c_str());
		if (file.fail())
			throw errorHandler(WRONG_FILE, confFile);
		initBlockNames();
		flatten(file);
		tokenize();
		parseBlock();
		file.close();
	}
	catch (const std::exception &e)
	{
		throw errorHandler(std::string(e.what()));
	}
}

std::map<std::string, serverConfig> configParser::getConfigs(void) { return hosts; }

bool configParser::checkBlocknames(std::string const &blockName)
{
	std::map<std::string, void(*)(std::stack<std::string>&)>::iterator res = blockNames.find(blockName);
	if (res != blockNames.end())
		return true;
	return false;
}

void configParser::extractBlockProp(size_t &i)
{
	std::string token;
	try
	{
		for (; i < conf.size(); i++)
		{
			if (conf[i] == ';')
			{
				if (token.empty())
					throw errorHandler(INVALID_INSTRUCTION, token);
				token += conf[i];
				tokens.push(token);
				token.clear();
			}
			else if (conf[i] == ']')
				throw errorHandler(MISSING_TOKEN, "[");
			else if (conf[i] == '}')
			{
				// tokens.push(token);
				// token.clear();
				return;
			}
			else if (conf[i] == '{')
				throw errorHandler (WRONG_TOKEN, "{");
			else if (conf[i] == '[')
				listToken(i, token);
			else if (!isspace(static_cast<unsigned char>(conf[i])))
				token += conf[i];
			else if (isspace(static_cast<unsigned char>(conf[i])))
			{
				if (!token.empty() && token[token.size() - 1] != ':' && token[token.size() - 1] != ';')
					throw errorHandler(INVALID_INSTRUCTION, token + " missing : or ;");
			}
		}
		throw errorHandler(MISSING_TOKEN, "}");
	}
	catch(const std::exception& e)
	{
		throw errorHandler(std::string(e.what()));
	}
}

std::time_t configParser::getTimeout() {return timeout;}

void configParser::addConfig()
{
	serverConfig host = configHandler::getHost();
	std::vector<std::string> ports = host.getHost().ports;
	std::string ip = host.getHost().addr;
	std::string name = host.getHost().hostname;
	for (size_t i = 0; i < ports.size(); i++)
	{
		LookUpKey key = std::make_pair(ip, ports[i]);
		std::map<LookUpKey, std::string>::iterator res = virtualHosts.find(key);
		if (res != virtualHosts.end())
		{
			if (res->second == name)
				throw errorHandler(INVALID_INSTRUCTION, " " + ip + ":" + ports[i] + " " + name);
		}
		virtualHosts[key] = name;
	}
	std::map<std::string, serverConfig>::iterator found = hosts.find(name);
	if (found != hosts.end())
		mergeConfigs(host, found->second);
	else
	{
		host.checkConfig();
		hosts[name] = host;
	}
	tokens.pop();
}

void configParser::mergeConfigs(serverConfig const & conf, serverConfig &origConf)
{
	std::map<std::string, t_location> locations = conf.getLocations();
	std::map<std::string, t_route>routes = conf.getRoutes();
	std::map<std::string, t_location>::iterator itL = locations.begin();
	for (; itL != locations.end(); ++itL)
		origConf.addLocation(itL->first, itL->second);
	std::map<std::string, t_route>::iterator itR = routes.begin();
	for (; itR != routes.end(); ++itR)
		origConf.addRoute(itR->first, itR->second);
}
