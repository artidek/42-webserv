#include "../includes/configUtils.hpp"
#include "../includes/errorHandler.hpp"
#include <sys/types.h>
#include <unistd.h>

bool	configUtils::isCurlBr(unsigned char c)
{
	if (c == '{' || c == '}')
		return (true);
	return (false);
}

void	configUtils::checkExt(std::string confFile)
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

bool configUtils::checkAddr(std::string addr)
{
	std::string buff;
	std::stringstream ss(addr);
	unsigned short i = 0;
	unsigned int addrPart;

	while (std::getline(ss, buff, '.'))
	{
		if (buff.empty())
			return false;
		std::stringstream toNum(buff);
		toNum >> addrPart;
		if (toNum.fail() || addrPart > 255)
			return false;
		i++;
	}
	if (i != 4)
		return false;
	return true;
}

bool configUtils::checkPort(std::string port)
{
	std::stringstream ss(port);
	unsigned int p;
	ss >> p;
	if (ss.fail())
		return false;
	if (p > 65535)
		return false;
	return true;
}

void configUtils::ifFile(std::string const &path)
{
	if (access(path.c_str(), R_OK) < 0)
		throw errorHandler(WRONG_FILE, path);
}