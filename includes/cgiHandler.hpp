#pragma once

#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include "serverConfig.hpp"
#include "requestHandler.hpp"
#include "responseHandler.hpp"
#include <iostream>
#include <sys/wait.h>
#include <stdlib.h>

class cgiHandler
{
	private:
		serverConfig config;
		t_request	request;
		std::string sendBuff;
		std::vector<std::string> env;
		std::vector<char*> envp;
		bool success;
		// cgiHandler(cgiHandler const &copy);
		// cgiHandler &operator=(cgiHandler const &copy);
		// void checkSuccess(void);
	public:
		cgiHandler(serverConfig const &conf, t_request const &req);
		~cgiHandler();
		// std::string const &getSendBuff(void) const;
		void run(std::string const &path);
		bool isSuccess(void) const;
		bool isCgiAllowed() const;
		std::string getPage(t_route const &route) const;
		bool checkPageExtension(std::string const &page) const;
		bool fileExist(std::string const &path) const;
		void setEnv(std::string const &page);
		void prepareEnvp(void);
};


#endif
