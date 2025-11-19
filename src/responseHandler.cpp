#include "../includes/responseHandler.hpp"
#include "../includes/configUtils.hpp"
#include <iostream>
#include <algorithm>

s_response::s_response(void)
{
	respCodes["Continue"] = 100;
	respCodes["OK"] = 200;
	respCodes["Created"] = 201;
	respCodes["Accepted"] = 202;
	respCodes["No Content"] = 204;
	respCodes["Multiple Choices"] = 300;
	respCodes["Moved Permanently"] = 301;
	respCodes["Found"] = 302;
	respCodes["Not Modified"] = 304;
	respCodes["Bad Request"] = 400;
	respCodes["Unauthorized"] = 401;
	respCodes["Forbidden"] = 403;
	respCodes["Not Found"] = 404;
	respCodes["Method Not Allowed"] = 405;
	respCodes["Not Acceptable"] = 406;
	respCodes["Request Timeout"] = 408;
	respCodes["Request To Large"] = 413;
	respCodes["Unsupported Media Type"] = 415;
	respCodes["Internal Server Error"] = 500;
	respCodes["Not Implemented"] = 501;
	respCodes["Bad Gateway"] = 502;
	respCodes["Service Unavailable"] = 503;
}

responseHandler::responseHandler(serverConfig const &config, t_request const &req) {

	runMethod[GET] = responseHandler::runGet;
	runMethod[POST] = responseHandler::runPost;
	runMethod[HEAD] = responseHandler::runHead;
	runMethod[DELETE] = responseHandler::runDelete;
	conf = config;
	request = req;
}

responseHandler::~responseHandler(void) {}

void responseHandler::allowedMethod(std::string const &root)
{
	t_location loc = conf.getLocations()[root];
	std::vector <std::string>::iterator res = std::find(loc.methods.begin(), loc.methods.end(), "GET");
	if (res == loc.methods.end())
	{
		resp.respCode = resp.respCodes["Method Not Allowed"];
		throw errorHandler("Method Not Allowed");
	}
}

void responseHandler::isRoute(std::string const &rt, t_route &route)
{
	try
	{
		route = conf.getRoute(rt);
	}
	catch(const std::exception& e)
	{
		resp.respCode = resp.respCodes["Not Found"];
		throw errorHandler("Not Found");
	}
	
}

void responseHandler::runGet(void)
{
	t_route route;
	try
	{
		isRoute(request.route, route);
		allowedMethod(route.newRoot);
	}
	catch(const std::exception& e)
	{
		throw errorHandler(std::string(e.what()));
	}
	
}

void responseHandler::runPost(void)
{

}

void responseHandler::runHead(void)
{

}

void responseHandler::runDelete(void)
{

}

void responseHandler::isMethod(std::string &mtd)
{
	std::string m = request.method;
	if (m == OPTIONS)
	{
		m = request.headers["Access-Control-Request-Method"];
		if (m != GET && m != POST && m != DELETE && m != HEAD)
		{
			resp.respCode = resp.respCodes["Bad Request"];
			throw errorHandler("Bad request");
		}
		else
			mtd = m;
	}
	else if (m == GET || m == POST || m == DELETE || m == HEAD)
		mtd = m;
	else
	{
		resp.respCode = resp.respCodes["Bad Request"];
		throw errorHandler("Bad request");
	}
}

void responseHandler::createResponce(void)
{
	std::string method;
	std::string loc;
	try
	{
		isMethod(method);
		runMethod[method]();
	}
	catch(const std::exception& e)
	{
		throw errorHandler(std::string(e.what()));
	}
	
}