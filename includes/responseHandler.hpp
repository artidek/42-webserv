#pragma once

#ifndef RESPONSE_HANDLER_H
#define RESPONSE_HANDLER_H

#include "requestHandler.hpp"
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
	std::map<std::string, int> respCodes;
	std::map<std::string, std::string> headers;
	std::string body;
	s_response(void);
} t_response;

class responseHandler
{
	private:
		std::map<std::string, void(*)(void)>runMethod;
		static serverConfig conf;
		static t_request request;
		static t_response resp;
		responseHandler(responseHandler const &copy);
		responseHandler &operator=(responseHandler const &copy);
		static void runGet();
		static void runPost();
		static void runHead();
		static void runDelete();
		void isMethod(std::string &mtd);
		static void isRoute(std::string const &rt, t_route &route);
		static void allowedMethod(std::string const &root);
	public:
		responseHandler(serverConfig const &config, t_request const &req);
		~responseHandler(void);
		void createResponce(void);
};

#endif