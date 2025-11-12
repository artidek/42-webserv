#include "../includes/configParser.hpp"
#include <fstream>
#include <iostream>

std::stack<std::string> configParser::tokens;
std::string configParser::flattened;
bool configParser::blockEnd = false;

bool isCurlBr(unsigned char c)
{
	if (c == '{' || c == '}')
		return true;
	return false;
}

void checkExt(std::string confFile)
{
	std::stringstream ss(confFile);
	std::string name, extension;

	if (std::getline(ss, name, '.') && std::getline(ss, extension))
	{
		if (extension == "conf")
			return;
		else
			throw errorHandler(WRONG_EXT, extension);
	}
	throw errorHandler(WRONG_EXT, "");
}

void configParser::flatten(std::ifstream const & file)
{
	std::ostringstream buffer;
	buffer << file.rdbuf();

	if (buffer.str().empty())
		throw errorHandler(CONFIG_EMPTY);
	flattened = buffer.str();
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
			tokens.push(token);
			return;
		}
		if (flattened[i] == ',' && !token.empty())
		{
			tokens.push(token);
			token.clear();
		}
		else if (!isspace(static_cast<unsigned char>(flattened[i])))
			token  += flattened[i];
	}
}

void configParser::tokenize(void)
{
	std::string token;
	for (int i = 0; flattened[i]; i++)
	{
		if(isCurlBr(flattened[i]))
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
		else if (!isspace(static_cast<unsigned char>(flattened[i])))
			token += flattened[i];
		else if (isspace(static_cast<unsigned char>(flattened[i])) && !token.empty())
		{
			tokens.push(token);
			token.clear();
		}
	}
}

std::vector<serverConfig> configParser::parseConfig(std::string confFile)
{
	std::vector<serverConfig> res;

	try
	{
		checkExt(confFile);
		std::ifstream file(confFile.c_str());
		if (file.fail())
			throw errorHandler(WRONG_FILE, confFile);
		flatten(file);
		tokenize();
	}
	catch(const std::exception& e)
	{
		throw errorHandler(std::string(e.what()));
	}
	return res;
}