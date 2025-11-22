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

typedef struct s_response
{
	int respCode;
	std::map<int, std::string> respCodes;
	std::map<std::string, std::string> headers;
	std::string body;
	s_response(void);
} t_response;

typedef struct s_request t_request;

class responseHandler
{
	private:
		bool isGetFile;
		bool emptyBody;
		std::string file;
		std::map<std::string, void (responseHandler::*)(void)>runMethod;
		serverConfig conf;
		t_request const &request;
	 	t_response resp;
		responseHandler(responseHandler const &copy);
		responseHandler &operator=(responseHandler const &copy);
		void runGet();
		void runPost();
		void runHead();
		void runDelete();
		void isMethod(std::string &mtd);
		void isRoute(std::string const &rt, t_route &route);
		void allowedMethod(std::string const &root);
		void ifGetFile(std::string const &rt, std::string &route);
		void fillResponseBody(std::string const &filePath);
		std::string eTag(std::string const &file);
		void fillSendBuffer(std::string &buffer);
		void sendToClient(size_t const &size, const char *buff, int const &fd); 
	public:
		responseHandler(serverConfig const &config, t_request const &req);
		~responseHandler(void);
		void createResponce(void);
		void sendResponse(int const &fd);
		t_response const getResponceData(void) const;
		void sendBad(int const &respCode, int const &fd);
		int  getRespCode() const;
};

#endif
