#pragma once

#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include "serverConfig.hpp"
#include "requestHandler.hpp"

class cgiHandler
{
private:
	serverConfig config;
	t_request	request;
	std::string sendBuff;
	std::vector<std::string>env;
	bool success;
	cgiHandler(cgiHandler const &copy);
	cgiHandler &operator=(cgiHandler const &copy);
	void setEnv(void);
	void checkSuccess(void);
public:
	cgiHandler(serverConfig const &conf, requestHandler &const req);
	~cgiHandler();
	std::string const &getSendBuff(void) const;
	void run(std::vector<std::string> envp);
	bool isSuccess(void) const;
};

#endif
