#pragma once

#ifndef CONFIG_HANDLER_H
#define CONFIG_HANDLER_H

#include <stack>
#include <string>
#include "../includes/serverConfig.hpp"

class configHandler
{
private:
	static serverConfig host;
	configHandler(void);
	configHandler(configHandler const &copy);
	configHandler &operator=(configHandler const &copy);
	~configHandler(void);
	static void fillAddr(t_host &newHost, std::string const &line);
	static void setMaxReqBody(t_host &newHost, std::string const &prop);
	static void setTimeout(t_host &newHost, std::string const &prop); 
public:
	static void fillHostConf(std::stack<std::string> &blockTokens);
	static void fillErrPg(std::stack<std::string> &blockTokens);
	static void fillRoute(std::stack<std::string> &blockTokens);
	static void fillLoc(std::stack<std::string> &blockTokens);
	static void fillCgiConf(std::stack<std::string> &blockTokens);
	static serverConfig getHost(void);
};


#endif