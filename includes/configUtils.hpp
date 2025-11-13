#pragma once

#ifndef CONFIG_UTILS_H
#define CONFIG_UTILS_H

#include <string>
#include "../includes/serverConfig.hpp"

class configUtils
{
	private:
		configUtils(void);
		configUtils(configUtils const &copy);
		configUtils &operator=(configUtils const &copy);
		~configUtils(void);
	public:
		static void	checkExt(std::string confFile);
		static bool	isCurlBr(unsigned char c);
		static bool checkAddr(std::string addr);
		static bool checkPort(std::string port);
		static void ifFile(std::string const &path);	
};

#endif