#include "../includes/configUtils.hpp"
#include "../includes/errorHandler.hpp"
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>

bool configUtils::isCurlBr(unsigned char c)
{
	if (c == '{' || c == '}')
		return (true);
	return (false);
}

void configUtils::checkExt(std::string confFile)
{
	std::stringstream ss(confFile);
	std::string name, extension;
	if (std::getline(ss, name, '.') && std::getline(ss, extension))
	{
		if (extension == "conf")
			return ;
		else
			throw errorHandler(WRONG_EXT, extension);
	}
	throw errorHandler(WRONG_EXT, "");
}

void configUtils::checkAddr(std::string addr)
{
	unsigned short	i;
	unsigned int	addrPart;

	std::string buff;
	std::stringstream ss(addr);
	i = 0;
	while (std::getline(ss, buff, '.'))
	{
		if (buff.empty())
			throw errorHandler(INVALID_INSTRUCTION, addr);
		std::stringstream toNum(buff);
		toNum >> addrPart;
		if (toNum.fail() || addrPart > 255)
			throw errorHandler(INVALID_INSTRUCTION, addr);
		i++;
	}
	if (i != 4)
		throw errorHandler(INVALID_INSTRUCTION, addr);
}

void configUtils::checkPort(std::string port)
{
	unsigned int	p;

	std::stringstream ss(port);
	ss >> p;
	if (ss.fail())
		throw errorHandler(INVALID_INSTRUCTION, port);
	if (p > 65535)
		throw errorHandler(INVALID_INSTRUCTION, port);
}

void configUtils::ifFile(std::string const &path)
{
	if (access(path.c_str(), R_OK) < 0)
		throw errorHandler(WRONG_FILE, path);
}

unsigned int configUtils::toNum(std::string s)
{
	unsigned int	num;

	std::stringstream ss(s);
	ss >> num;
	if (ss.fail())
		throw errorHandler(INVALID_INSTRUCTION, s);
	return (num);
}

void configUtils::ifDir(std::string const &path)
{
	DIR	*dr;

	dr = opendir(path.c_str());
	if (dr)
		closedir(dr);
	else
		throw errorHandler(INVALID_DIR, path);
}

void configUtils::concatFilePath(std::string &filePath,
	std::string const &fileName)
{
	if (filePath[filePath.size() - 1] == '/')
		filePath += fileName;
	else
		filePath += ("/" + fileName);
}

bool configUtils::getFromStack(std::string &s,
	std::stack<std::string> &blockNames)
{
	if (blockNames.empty())
		return (false);
	std::string top = blockNames.top();
	s = top.substr(0, top.size() - 1);
	blockNames.pop();
	return (true);
}

bool configUtils::onOff(std::string const &prop)
{
	if (prop == "on")
		return (true);
	else if (prop == "off")
		return (false);
	else
		throw errorHandler(INVALID_INSTRUCTION, prop);
}

void configUtils::getFromList(t_location &loc,
	std::stack<std::string> &blockTokens)
{
	std::string setter;
	while (!blockTokens.empty())
	{
		setter = blockTokens.top();
		if (setter == "]")
		{
			blockTokens.pop();
			return ;
		}
		if (setter[setter.size() - 1] == ',')
			setter = setter.substr(0, setter.size() - 1);
		if (setter == "GET" || setter == "HEAD" || setter == "POST"
			|| setter == "DELETE")
			loc.methods.push_back(setter);
		else
			throw errorHandler(INVALID_INSTRUCTION, setter);
		blockTokens.pop();
	}
}

void	configUtils::getFromList(t_cgi &cgi, std::stack<std::string> &blockTokens)
{
	std::string setter;
	while (!blockTokens.empty())
	{
		setter = blockTokens.top();
		if (setter == "]")
		{
			blockTokens.pop();
			return ;
		}
		if (setter[setter.size() - 1] == ',')
			setter = setter.substr(0, setter.size() - 1);
		std::vector<std::string>::iterator res;
		res = std::find(cgi.extensions.begin(), cgi.extensions.end(),
				setter.c_str());
		if (res == cgi.extensions.end())
			cgi.extensions.push_back(setter);
		blockTokens.pop();
	}
}

void configUtils::ifPage(std::string const &path, std::string const &page)
{
	if (page == "none")
		return;
	std::string pagePath = path;
	try
	{
		concatFilePath(pagePath, page);
		ifFile(pagePath);
	}
	catch(const std::exception& e)
	{
		throw errorHandler(std::string(e.what()));
	}
	
}

std::string configUtils::trim (std::string const &src, std::string const &set)
{
	std::string res = src;
	for (int i = 0; set[i]; i++)
	{
		size_t first = res.find_first_not_of(set[i]);
		size_t last = res.find_last_not_of(set[i]);
		res = res.substr(first, last);
	}
	return res;
}