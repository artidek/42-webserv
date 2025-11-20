#include "../includes/responseHandler.hpp"
#include "../includes/configUtils.hpp"
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sys/stat.h>
#include <sys/socket.h>

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

bool responseHandler::isGetFile = false;
std::string responseHandler::file;
t_request responseHandler::request;
t_response responseHandler::resp;
serverConfig responseHandler::conf;

responseHandler::responseHandler(serverConfig const &config, t_request const &req) {

	runMethod[GET] = responseHandler::runGet;
	runMethod[POST] = responseHandler::runPost;
	runMethod[HEAD] = responseHandler::runHead;
	runMethod[DELETE] = responseHandler::runDelete;
	conf = config;
	request = req;
}

responseHandler::~responseHandler(void) {}

std::string responseHandler::eTag(std::string const &file)
{
	struct stat st;
    if (stat(file.c_str(), &st) != 0) {
        return "";  // file not found
    }

    std::stringstream ss;
    ss << "\""
       << std::hex << st.st_ino        // inode
       << "-" << st.st_size            // file size
       << "-" << st.st_mtime           // last modified timestamp
       << "\"";

    return ss.str();
}

t_response const responseHandler::getResponceData(void) const
{
	return resp;
}

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

void responseHandler::ifGetFile(std::string const &rt, std::string &route)
{
	size_t split = rt.find_last_of("/");
	file = rt.substr(split + 1);
	if (file.find(".") != std::string::npos)
	{
		isGetFile = true;
		route = rt.substr(0, split);
		return;
	}
	route = rt;
}

void responseHandler::isRoute(std::string const &rt, t_route &route)
{
	std::string cleanedRoute;
	ifGetFile(rt, cleanedRoute);
	try
	{

		route = conf.getRoute(cleanedRoute);
		if (isGetFile)
		{
			std::string filePath = route.newRoot + file;
			configUtils::ifFile(filePath);
		}
	}
	catch(const std::exception& e)
	{
		resp.respCode = resp.respCodes["Not Found"];
		throw errorHandler("Not Found");
	}

}

void responseHandler::fillResponseBody(std::string const & filePath)
{
	std::fstream file(filePath.c_str());
	if (file.fail())
	{
		resp.respCode = resp.respCodes["Internal Server Error"];
		file.close();
		throw errorHandler("Internal Server Error");
	}
	else
	{
		std::stringstream ss;
		ss << file.rdbuf();
		resp.body = ss.str();
	}
	file.close();
}

void responseHandler::runGet(void)
{
	t_route route;
	try
	{
		isRoute(request.route, route);
		allowedMethod(route.newRoot);
		std::stringstream ss(route.response);
		ss >> resp.respCode;
		if (isGetFile)
			fillResponseBody(route.newRoot + file);
		else
		{
			if (route.page == "none")
			{
				resp.respCode = resp.respCodes["Forbidden"];
				throw errorHandler("Forbiddden");
			}
			else
				fillResponseBody(route.newRoot + route.page);
		}
		resp.headers["Server:"] = SRV;
		resp.headers["Date:"] = configUtils::getDateTime();
		resp.headers["Content-Type:"] = request.headers["Content-Type:"];
		ss.clear();
		ss << resp.body.size();
		resp.headers["Content-Length:"] = ss.str();
		resp.headers["Connection:"] = "keep-alive";
		resp.headers["ETag:"] = eTag(route.newRoot + route.page);
		resp.headers["Accept-Ranges:"] = "bytes";

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

void responseHandler::fillSendBuffer(std::string &buffer)
{
	std::stringstream ss;
	ss << "HTTP/1.1 " << resp.respCode << " OK" << "\r\n";
	std::map<std::string, std::string>::iterator it = resp.headers.begin();
	for (; it != resp.headers.end(); ++it)
		ss << it->first << " " << it->second << "\r\n";
	ss << "\r\n";
	ss << resp.body;
	buffer = ss.str();
}

void responseHandler::sendResponse(int const &fd)
{
	std::string buffer;
	fillSendBuffer(buffer);
	size_t len = buffer.size();
	size_t total = 0;
	const char *buffP = buffer.c_str();
	while (total < len)
	{
		int writeBytes = send(fd, buffP + total, len - total, 0);
		if (writeBytes < 0)
		{
			if (errno == EINTR) continue;
			else if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			else 
				throw errorHandler("send failed");
		}
		if (writeBytes == 0)
			throw errorHandler("peer closed");
		total += writeBytes;
	}
}
