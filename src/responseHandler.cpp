#include "../includes/responseHandler.hpp"
#include "../includes/requestHandler.hpp"
#include "../includes/configUtils.hpp"
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sys/stat.h>
#include <sys/socket.h>

s_response::s_response(void)
{
	respCodes[100] = "Continue";
	respCodes[200] = "OK";
	respCodes[201] = "Created";
	respCodes[202] = "Accepted";
	respCodes[204] = "No Content";
	respCodes[300] = "Multiple Choices";
	respCodes[301] = "Moved Permanently";
	respCodes[302] = "Found";
	respCodes[304] = "Not Modified";
	respCodes[400] = "Bad Request";
	respCodes[401] = "Unauthorized";
	respCodes[403] = "Forbidden";
	respCodes[404] = "Not Found";
	respCodes[405] = "Method Not Allowed";
	respCodes[408] = "Request Timeout";
	respCodes[413] = "Request To Large";
	respCodes[500] = "Internal Server Error";
	respCodes[501] = "Not Implemented";
	respCodes[502] = "Bad Gateway";
	respCodes[503] = "Service Unavailable";
}

responseHandler::responseHandler(serverConfig const &config, t_request const &req) : request(req) {

	runMethod[GET] = &responseHandler::runGet;
	runMethod[POST] = &responseHandler::runPost;
	runMethod[HEAD] = &responseHandler::runHead;
	runMethod[DELETE] = &responseHandler::runDelete;
	conf = config;
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
		resp.respCode = 405;
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
	isGetFile = false;
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
		resp.respCode = 404;
		throw errorHandler("Not Found");
	}

}

void responseHandler::fillResponseBody(std::string const & filePath)
{
	std::fstream file(filePath.c_str());
	if (file.fail()) {
    	resp.respCode = 500;
    	throw errorHandler("Internal Server Error");
	}
	std::stringstream ss;
	if (resp.respCode != 204)
	{
		ss << file.rdbuf();
		resp.body = ss.str();
		emptyBody = false;
		return;
	}
	emptyBody = true;
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
				resp.respCode = 403;
				throw errorHandler("Forbiddden");
			}
			else
				fillResponseBody(route.newRoot + route.page);
		}
		resp.headers["Server:"] = SRV;
		resp.headers["Date:"] = configUtils::getDateTime();
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
		std::map<std::string, std::string> headers = request.headers;
		m = headers["Access-Control-Request-Method"];
		if (m != GET && m != POST && m != DELETE && m != HEAD)
		{
			resp.respCode = 400;
			throw errorHandler("Bad request");
		}
		else
			mtd = m;
	}
	else if (m == GET || m == POST || m == DELETE || m == HEAD)
		mtd = m;
	else
	{
		resp.respCode = 400;
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
		(this->*runMethod[method])();
	}
	catch(const std::exception& e)
	{
		throw errorHandler(std::string(e.what()));
	}

}

void responseHandler::fillSendBuffer(std::string &buffer)
{
	std::stringstream ss;
	std::map<int , std::string>::iterator res = resp.respCodes.find(resp.respCode);
	ss << "HTTP/1.1 " << resp.respCode << " " << res->second << "\r\n";
	std::map<std::string, std::string>::iterator it = resp.headers.begin();
	for (; it != resp.headers.end(); ++it)
		ss << it->first << " " << it->second << "\r\n";
	ss << "\r\n";
	if (!emptyBody)
		ss << resp.body;
	buffer = ss.str();
}

void responseHandler::sendToClient(size_t const &size, const char *buff, int const &fd)
{
	size_t total = 0;
	while (total < size)
	{
		int writeBytes = send(fd, buff + total, size - total, 0);
		if (writeBytes < 0)
		{
			if (errno == EINTR) continue;
			else if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			else 
				throw errorHandler("Send failed");
		}
		if (writeBytes == 0)
			throw errorHandler("Peer closed");
		total += writeBytes;
	}
}

void responseHandler::sendResponse(int const &fd)
{
	std::string buffer;
	fillSendBuffer(buffer);
	try
	{
		sendToClient(buffer.size(), buffer.c_str(), fd);
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
}

void responseHandler::sendBad(int const &respCode, int const &fd)
{
	resp.respCode = respCode;
	resp.headers["Date:"] = configUtils::getDateTime();
	resp.headers["Content-Length:"] = "0";
	resp.headers["Connection:"] = "close";
	if (respCode != 408 && respCode != 413)
		fillResponseBody(conf.getErrorPage(respCode));
	std::string buff;
	fillSendBuffer(buff);
	sendToClient(buff.size(), buff.c_str(), fd);
}

int  responseHandler::getRespCode(void) const {return resp.respCode;}