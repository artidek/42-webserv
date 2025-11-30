#pragma once

#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include "serverConfig.hpp"
#include "responseHandler.hpp"
#include <stack>
#include <ctime>


#define BUFFER_SIZE 8192

typedef struct s_reqBody
{
	std::string fileName;
	std::string content;
	bool empty(void);
} t_reqBody;

typedef struct s_request
{
	std::string method;
	std::string route;
	std::string query;
	std::string page;
	std::string path_info;
	t_reqBody body;
	std::map<std::string, std::string> headers;
} t_request;

class requestHandler
{
	private:
		serverConfig _host;
		std::string _rawData;
		std::string _endBody;
		static std::map<int, double>timeLog;
		std::stack<std::string> _tokens;
		static std::map<std::string, std::string> _headers;
		t_request _request;
		static std::map<std::string, std::string> initHeaders(void);
		void tokenize(void);
		void fillHeader(std::string headerProp, std::string headerVal);
		t_reqBody fillReqBody(void);
		void fillMethodRoute(std::string headerProp);
		void getFileName(t_reqBody &reqBody, std::string value);
		void setBodyEnd(std::string token);
		bool isBodyHeader(std::string &h, std::string &v, std::string const &token);
		void checkTimeout(int fd, double sec);
		void parseRoute(std::string const &rawRoute);
		void extractPathInfo(std::stringstream const &ss);
		void buildRoute(std::vector<std::string> const &tokens);
	public:
		requestHandler(void);
		requestHandler(serverConfig const &copy);
		requestHandler(requestHandler const &copy);
		requestHandler &operator=(requestHandler const &copy);
		~requestHandler(void);
		void read(int const &fd);
		void parse(void);
		std::string const &getRawData(void) const;
		t_request const getReqData(void) const;
		bool requestComplete(void);
		serverConfig const getConfig(void) const;
		std::string const getEndBody(void) const;
		std::stack<std::string> const getTokens(void) const;
		void addToTimeLog(int fd, double sec);
		void removeFromTimeLog(int const & fd);
};

std::ostream &operator<< (std::ostream &o, requestHandler const &req);

#endif
