#include "../includes/configUtils.hpp"
#include "../includes/errorHandler.hpp"
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>
#include <ctime>
#include <cstring>
#include <iostream>

const char * t_dayMonth::days[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char * t_dayMonth::months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
const int configUtils::respCodes[20] = {100, 200, 201, 202, 204, 300, 301, 302, 304, 400, 401, 403, 404, 405, 408, 413, 500, 501, 502, 503};

bool configUtils::isCurlBr(unsigned char c)
{
	if (c == '{' || c == '}')
		return (true);
	return (false);
}

void configUtils::checkExt(std::string confFile)
{
	size_t found = confFile.rfind(".");
	if (found == std::string::npos)
		throw errorHandler(WRONG_FILE, confFile);
	std::string ext = confFile.substr(found + 1);
	if (ext.empty() || ext != "conf")
		throw errorHandler(WRONG_EXT, confFile);
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
		addrPart = toNum(buff);
		if (addrPart > 255)
			throw errorHandler(INVALID_INSTRUCTION, addr);
		i++;
	}
	if (i != 4)
		throw errorHandler(INVALID_INSTRUCTION, addr);
}

void configUtils::checkPort(std::string port)
{
	unsigned int	p;

	if (port.size() < 2)
		throw errorHandler (INVALID_INSTRUCTION, port);
	p = toNum(port);
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
	if (s.size() > 1 && s[0] == '0')
		throw errorHandler (INVALID_INSTRUCTION, s);
	for (size_t i = 0; i < s.size(); i++)
	{
		if (!std::isdigit(s[i]))
			throw errorHandler(INVALID_INSTRUCTION, s);
	}
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

void configUtils::getFromList(t_route &route,
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
			route.methods.push_back(setter);
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

void configUtils::getFromList(std::vector<std::string> &list, std::stack<std::string> &blockTokens)
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
		res = std::find(list.begin(), list.end(),
				setter.c_str());
		if (res == list.end())
			list.push_back(setter);
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
	size_t first = res.find_first_not_of(set);
	if (first == std::string::npos)
		return "";
	size_t last = res.find_last_not_of(set);
	res = res.substr(first, last - first + 1);
	return res;
}

std::string configUtils::formatTime(int const &tm)
{
	std::stringstream ss;
	if (tm / 10 == 0)
	{
		ss << 0 << tm;
		return ss.str();
	}
	ss << tm;
	return ss.str();
}

std::string configUtils::getDateTime(void)
{
	std::time_t now = std::time(NULL);
    std::tm *ptm = std::gmtime(&now);
	std::stringstream ss;
	t_dayMonth dM;

    int year   = ptm->tm_year + 1900;
    int month  = ptm->tm_mon;
    int day    = ptm->tm_mday;
    int wday   = ptm->tm_wday;   // 0 = Sunday
    int hour   = ptm->tm_hour;
    int minute = ptm->tm_min;
    int second = ptm->tm_sec;
	ss << dM.days[wday] << ", " << day << " " << dM.months[month] << " " << year << " " << formatTime(hour) << ":" << formatTime(minute) << ":" << formatTime(second) << " GMT";
	return ss.str();
}

std::time_t configUtils::getTime(void)
{
	std::time_t now = std::time(NULL);
    return now;
}

std::string configUtils::buildPath(std::string const &path, std::string const &name)
{
	std::string res;
	if (path[path.size() - 1] != '/')
		res = path + "/" + name;
	else
		res = path + name;
	return res;
}

bool configUtils::isResCode(int const &code)
{
	for (int i = 0; i < 20; i++)
	{
		if (code == respCodes[i])
			return true;
	}
	return false;
}