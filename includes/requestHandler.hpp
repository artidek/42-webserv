#pragma once

#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include "serverConfig.hpp"
#include <stack>


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
	t_reqBody body;
	std::map<std::string, std::string> headers;
	bool empty(void);
} t_request;

class requestHandler
{
	private:
		serverConfig _host;
		std::string _rawData;
		std::string _endBody;
		std::stack<std::string> _tokens;
		static std::map<std::string, std::string> _headers;
		t_request _request;
		requestHandler(void);
		requestHandler(requestHandler const &copy);
		requestHandler &operator=(requestHandler const &copy);
		static std::map<std::string, std::string> initHeaders(void);
		void tokenize(void);
		void fillHeader(std::string headerProp, std::string headerVal);
		t_reqBody fillReqBody(void);
		void fillMethodRoute(std::string headerProp);
		void getFileName(t_reqBody &reqBody, std::string value);
		void setBodyEnd(std::string token);
	public:
		requestHandler(serverConfig const &copy);
		~requestHandler(void);
		void read(int const &fd);
		void parse(void);
		std::string const &getRawData(void) const;
		t_request getReqData(void) const;
};

#endif