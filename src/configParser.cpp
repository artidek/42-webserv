#include "../includes/configParser.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include "../includes/configUtils.hpp"
#include "../includes/configHandler.hpp"

std::stack<std::string> configParser::tokens;
std::vector<serverConfig> configParser::hosts;
std::map<std::string,
	void (*)(std::stack<std::string> &)> configParser::blockNames;
std::string configParser::flattened;
std::string configParser::blockProp;
bool configParser::blockEnd = false;

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

void configParser::listToken(int &i)
{
	std::string token;
	for (; flattened[i]; i++)
	{
		if (flattened[i] == ']')
		{
			if (!token.empty())
			{
				tokens.push(token);
				token.clear();
			}
			token += flattened[i];
			i++;
			tokens.push(token);
			return ;
		}
		if (flattened[i] == ',' && !token.empty())
		{
			tokens.push(token);
			token.clear();
		}
		else if (flattened[i] == '[' || flattened[i] == '{'
			|| flattened[i] == '}')
			throw errorHandler(MISSING_TOKEN, "]");
		else if (!isspace(static_cast<unsigned char>(flattened[i])))
			token += flattened[i];
	}
}

void configParser::tokenize(void)
{
	std::string token;
	try
	{
		for (int i = 0; flattened[i]; i++)
		{
			if (configUtils::isCurlBr(flattened[i]) && token.empty())
			{
				token += flattened[i];
				tokens.push(token);
				token.clear();
			}
			else if (flattened[i] == '[')
			{
				token += flattened[i];
				tokens.push(token);
				token.clear();
				i++;
				listToken(i);
			}
			else if(flattened[i] == ']')
				throw errorHandler(MISSING_TOKEN, "[");
			else if (!isspace(static_cast<unsigned char>(flattened[i])) && !configUtils::isCurlBr(flattened[i]))
				token += flattened[i];
			else if (isspace(static_cast<unsigned char>(flattened[i]))
				&& !token.empty())
			{
				tokens.push(token);
				token.clear();
			}
			else if (flattened[i] == '{')
			{
				tokens.push(token);
				tokens.push("{");
				token.clear();
				i++;
			}
		}
	}
	catch (const std::exception &e)
	{
		throw errorHandler(std::string(e.what()));
	}
}

void configParser::startBlock(void)
{
	if (tokens.top() == "}")
	{
		tokens.pop();
		if (tokens.top() == "}")
		{
			blockEnd = true;
			tokens.pop();
			return ;
		}
	}
	throw errorHandler(MISSING_TOKEN, "}");
}

void configParser::checkBlock(std::stack<std::string> &blockTokens)
{
	std::map<std::string, void (*)(std::stack<std::string> &)>::iterator res;
	std::string token = tokens.top();
	try
	{
		res = blockNames.find(token);
		if (res == blockNames.end())
		{
			blockProp = token;
			tokens.pop();
			token = tokens.top();
			res = blockNames.find(token);
			if (res == blockNames.end() && token != "}")
				throw errorHandler(INVALID_INSTRUCTION, token);
			else if (res == blockNames.end())
				throw errorHandler(INVALID_INSTRUCTION, blockProp);
			else
				res->second(blockTokens);
		}
		else
			res->second(blockTokens);
		tokens.pop();
	}
	catch (const std::exception &e)
	{
		throw errorHandler(std::string(e.what()));
	}
}

void configParser::parseList(std::stack<std::string> &blockTokens)
{
	std::string token = tokens.top();
	while (!tokens.empty())
	{
		if (token == "[")
		{
			blockTokens.push(token);
			tokens.pop();
			return ;
		}
		blockTokens.push(token);
		tokens.pop();
		token = tokens.top();
	}
	throw errorHandler(MISSING_TOKEN, "[");
}

 void configParser::parseBlock(void)
{
	char	last;

	std::stack<std::string> blockTokens;
 	std::string token;
	initBlockNames();
	try
 	{
		startBlock();
 		token = tokens.top();
 		while (!tokens.empty() && token != "host")
 		{
			last = token[token.size() - 1];
			//std::cout << "token " << token << "last " << last << "\n";
 			if (last == ':' || last == ';')
 			{
 				blockTokens.push(token);
				tokens.pop();
 			}
 			else if (last == ']')
 				parseList(blockTokens);
 			else if (last == '[')
 				throw errorHandler(MISSING_TOKEN, "]");
			else if (last == '}')
 			{
 				if(blockEnd)
				 	throw errorHandler(MISSING_TOKEN, "{");
 				blockEnd = true;
 				tokens.pop();
 			}
 			else if (last == '{')
			{
				if(blockEnd == false && !blockTokens.empty())
				{
					throw errorHandler(MISSING_TOKEN, "}");
				}
 				tokens.pop();
 				if (tokens.top() != "host")
					checkBlock(blockTokens);
 				else
 					tokens.pop();
 				blockEnd = false;
 			}
			else
				throw errorHandler(INVALID_INSTRUCTION, token);
 			if (!tokens.empty())
				token = tokens.top();
		}
		if (!tokens.empty())
		{
			std::cout << "token not empty " << tokens.top() << std::endl;
			throw errorHandler(MISSING_TOKEN, "{");
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
		flatten(file);
		tokenize();
		while (!tokens.empty())
		{
			parseBlock();
			hosts.push_back(configHandler::getHost());
		}
	}
	catch (const std::exception &e)
	{
		throw errorHandler(std::string(e.what()));
	}
}