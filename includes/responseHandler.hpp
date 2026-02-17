#pragma once

#ifndef RESPONSE_HANDLER_H
#define RESPONSE_HANDLER_H

#include "serverConfig.hpp"

#define SRV_NM "Server:"
#define DT "Date:"
#define CONT_TP "Content-Type:"
#define CONT_LEN "Content-Length:"
#define LST_MOD "Last-Modified:"
#define CONN "Connection:"
#define ET "ETag:"
#define ACPT_RNG "Accept-Ranges:"
#define LOC "Location:"

class requestHandler;

typedef struct s_response
{
	int respCode;
	std::map<int, std::string> respCodes;
	std::map<std::string, std::string> headers;
	std::string body;
	s_response(void);
} t_response;

class responseHandler
{
	private:
		bool emptyBody;
		bool sendComplete;
		bool cgi;
		size_t size;
		size_t total;
		std::string path;
		std::string file;
		std::string buffer;
		std::map<std::string, void (responseHandler::*)(void)>runMethod;
		serverConfig conf;
		requestHandler request;
	 	t_response resp;
		t_route route;
		void runGet();
		void runPost();
		void runHead();
		void runDelete();
		void isMethod(std::string &mtd);
		void isRoute(t_route &route);
		void allowedMethod(std::string const &root);
		void fillResponseBody(std::string const &filePath);
		std::string eTag(std::string const &file);
		void fillSendBuffer();
		void validFile(t_route const &route);
		bool isCgi();
		void fillHeaders(std::string connection, std::string contLen);
		std::string getExt (std::string const &path);
		void getList();
		void uniqueName(std::string &flName);
		void runCgi(std::vector<std::string>env);
		bool checkNone(std::string const &path);
	public:
		responseHandler ();
		responseHandler(serverConfig const &config, requestHandler const &req);
		responseHandler(responseHandler const &copy);
		responseHandler &operator=(responseHandler const &copy);
		~responseHandler(void);
		void createResponce(std::vector<std::string> env);
		void sendResponse(int const &fd);
		void sendToClient(int const &fd);
		t_response const getResponceData(void) const;
		void sendBad(int const &respCode, int const &fd);
		int  getRespCode() const;
		bool responseComplete(void);
		int findRespCode(std::string const &err);
		requestHandler const & getRequest() const;
		size_t getSize() const;
		size_t getTotal() const;
		std::string const getBuffer() const;
		bool getComplete() const;
};

#endif
