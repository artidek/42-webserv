#pragma once

#ifndef CONFIG_UTILS_H
#define CONFIG_UTILS_H

#include <string>
#include "../includes/serverConfig.hpp"
#include <dirent.h>
#include <stack>

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
		static unsigned int toNum(std::string s);
		static void ifDir(std::string const &path);
		static void concatFilePath(std::string &filePath, std::string const &fileName);
		static bool getFromStack(std::string &s, std::stack<std::string> &blockNames);
		static void getFromList(t_location &loc, std::stack<std::string> &blockTokens);
		static void getFromList(t_cgi &cgi, std::stack<std::string> &blockTokens);
		static bool onOff(std::string const &prop);
};
#endif