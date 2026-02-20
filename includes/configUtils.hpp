#pragma once

#ifndef CONFIG_UTILS_H
#define CONFIG_UTILS_H

#include <string>
#include "../includes/serverConfig.hpp"
#include <dirent.h>
#include <stack>
#include <ctime>

typedef struct s_dayMonth
{
	static const char  *days[7];
	static const char  *months[12];
} t_dayMonth;

class configUtils
{
	private:
		static const int respCodes[20];
		configUtils(void);
		configUtils(configUtils const &copy);
		configUtils &operator=(configUtils const &copy);
		~configUtils(void);
	public:
		static void	checkExt(std::string confFile);
		static bool	isCurlBr(unsigned char c);
		static void checkAddr(std::string addr);
		static void checkPort(std::string port);
		static void ifFile(std::string const &path);
		static unsigned int toNum(std::string s);
		static void ifDir(std::string const &path);
		static void concatFilePath(std::string &filePath, std::string const &fileName);
		static bool getFromStack(std::string &s, std::stack<std::string> &blockNames);
		static void getFromList(t_route &route, std::stack<std::string> &blockTokens);
		static void getFromList(t_cgi &cgi, std::stack<std::string> &blockTokens);
		static void getFromList(std::vector<std::string> &list, std::stack<std::string> &blockTokens);
		static bool onOff(std::string const &prop);
		static void ifPage(std::string const &path, std::string const &page);
		static std::string trim (std::string const &src, std::string const & set);
		static std::string formatTime(int const &tm);
		static std::string getDateTime(void);
		static std::time_t getTime(void);
		static std::string buildPath(std::string const &path, std::string const &name);
		static bool isResCode(int const &code);
};
#endif