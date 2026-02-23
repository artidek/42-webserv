#pragma once

#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include "serverConfig.hpp"
#include <stack>
#include <ctime>


#define BUFFER_SIZE  65536

class responseHandler;
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
	t_reqBody body;
	std::map<std::string, std::string> headers;
} t_request;

class requestHandler
{
	private:
		int _readLen;
		serverConfig _host;
		std::string _rawData;
		std::string _endBody;
		std::string _readHeaders;
		std::stack<std::string> _tokens;
		static std::map<std::string, std::string> _headers;
		t_request _request;
		size_t _contLen;
		size_t _totalSize;
		std::map<std::string, serverConfig> configs;
		static std::map<std::string, std::string> initHeaders(void);
		std::vector<char> buffChunks;
		void tokenize(void);
		void fillHeader(std::string headerProp, std::string headerVal);
		void fillReqBody();
		void fillMethodRoute(std::string headerProp);
		void getFileName(t_reqBody &reqBody, std::string value);
		void setBodyEnd();
		void setContLen();
		bool isBodyHeader(std::string &h, std::string &v, std::string const &token);
		bool _isHeader;
		void parseRoute(std::string const &rawRoute);
		void fillReqBodyApp();
		void isHeaders();
		bool doneReading();
		std::string combine();
		void getRoutePage(std::string upToLastSlash, std::string parseRoute, size_t lastSlash);
	public:
		requestHandler(void);
		requestHandler(std::map<std::string, serverConfig> const &hosts);
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
		size_t getContLen() const;
		int getReadLen() const;
		bool getIsheader() const;
		std::string const getReadHeader() const;
		std::vector<char> const getBufChunks() const;
		size_t getTotalSize() const;
		std::map<std::string, serverConfig> getConfigs() const;
};

std::ostream &operator<< (std::ostream &o, requestHandler const &req);

#endif
