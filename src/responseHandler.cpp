#include "../includes/responseHandler.hpp"
#include "../includes/requestHandler.hpp"
#include "../includes/configUtils.hpp"
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <dirent.h>

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
	emptyBody = false;
	sendComplete = false;
	cgi = false;
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
	std::vector <std::string>::iterator res = std::find(loc.methods.begin(), loc.methods.end(), request.method);
	if (res == loc.methods.end())
	{
		resp.respCode = 405;
		throw errorHandler(resp.respCodes[405]);
	}
}

void responseHandler::isRoute(t_route &route)
{
	try
	{
		route = conf.getRoute(request.route);
	}
	catch(const std::exception& e)
	{
		resp.respCode = 404;
		throw errorHandler(resp.respCodes[404]);
	}

}

void responseHandler::fillResponseBody(std::string const & filePath)
{
	std::fstream file(filePath.c_str());
	if (file.fail()) {
    	resp.respCode = 500;
    	throw errorHandler(resp.respCodes[500]);
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
	std::stringstream ss;
	try
	{
		if (path.substr(path.size() - 4) == "none")
		{
			if (conf.getLocation(route.newRoot).enableListing)
				getList();
			else
			{
				resp.respCode = 403;
				throw errorHandler(resp.respCodes[403]);
			}
		}
		else if (isCgi())
		{
			cgi = true;
			//cgi handler goes here
		}
		else
		{
			if (access(path.c_str(), R_OK) == 0)
				fillResponseBody(path);
			else
			{
				resp.respCode = 403;
				throw errorHandler(resp.respCodes[403]);
			}
		}
		ss << resp.body.size();
		fillHeaders("keep-alive", ss.str());
	}
	catch(const std::exception& e)
	{
		throw errorHandler(std::string(e.what()));
	}

}

void responseHandler::runPost(void)
{
	try
	{
		if (path.substr(path.size() - 4) == "none")
		{
			if (conf.getLocation(route.newRoot).enableUpload)
			{
				std::string filename = request.body.fileName;
				filename = configUtils::trim(filename, "\"");
				uniqueName(filename);
				std::string path = route.newRoot;
				if (path[path.size() - 1] != '/')
				{
					path.resize(path.size() + 1);
					path += '/';
				}
				path += filename;
				std::fstream file(path.c_str(), std::ios::out);
				file << request.body.content;
				fillResponseBody("etc/error/success.html");
			}
			else
			{
				resp.respCode = 403;
				throw errorHandler(resp.respCodes[403]);
			}
		}
		if (isCgi())
		{
			cgi = true;
			//cgi handler goes here
		}
	}
	catch(const std::exception& e)
	{
		throw errorHandler(std::string(e.what()));
	}
}

void responseHandler::runHead(void)
{
	t_route route;
	try
	{
		isRoute(route);
		allowedMethod(route.newRoot);
		resp.respCode = 204;
		if (route.page == "none")
		{
			resp.respCode = 403;
			throw errorHandler("Forbiddden");
		}
		resp.headers["Server:"] = SRV;
		resp.headers["Date:"] = configUtils::getDateTime();
		resp.headers["Content-Length:"] = "0";
		resp.headers["Connection:"] = "close";
		resp.headers["ETag:"] = eTag(route.newRoot + route.page);
		resp.headers["Accept-Ranges:"] = "bytes";

	}
	catch(const std::exception& e)
	{
		throw errorHandler(std::string(e.what()));
	}
}

void responseHandler::runDelete(void)
{
	t_route route;
	std::string path;
	try
	{
		if (isCgi())
		{
			cgi = true;
			//cgi handler goes here
		}
		else
		{
			//upload logic goes here
		}
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
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
			throw errorHandler(resp.respCodes[400]);
		}
		else
			mtd = m;
	}
	else if (m == GET || m == POST || m == DELETE || m == HEAD)
		mtd = m;
	else
	{
		resp.respCode = 400;
		throw errorHandler(resp.respCodes[400]);
	}
}

void responseHandler::createResponce(void)
{
	std::string method;
	try
	{
		isMethod(method);
		isRoute(route);
		allowedMethod(route.newRoot);
		if (route.newRoot == "none" && request.page.empty())
		{
			resp.respCode = 500;
			throw errorHandler(resp.respCodes[500]);
		}
		else
		{
			if (!request.page.empty())
				path = configUtils::buildPath(route.newRoot, request.page);
			else
				path = configUtils::buildPath(route.newRoot,route.page);
		}
		std::stringstream ss(route.response);
		ss >> resp.respCode;
		(this->*runMethod[method])();
	}
	catch(const std::exception& e)
	{
		std::string err(e.what());
		if (err.find("No data available") != std::string::npos)
			resp.respCode = 500;
		throw errorHandler(std::string(err));
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
	if (std::string(buff).size() == total)
		sendComplete = true;
}

void responseHandler::sendResponse(int const &fd)
{
	std::string buffer;
	if (!cgi)
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
	resp.headers["Connection:"] = "close";
	if (respCode != 408 && respCode != 413)
	{
		std::stringstream ss;
		fillResponseBody(conf.getErrorPage(respCode));
		ss << resp.body.size();
		ss >> resp.headers["Content-Length:"];
	}
	else
		resp.headers["Content-Length:"] = "0";
	std::string buff;
	fillSendBuffer(buff);
	sendToClient(buff.size(), buff.c_str(), fd);
}

int  responseHandler::getRespCode(void) const {return resp.respCode;}

bool responseHandler::responseComplete(void) {return sendComplete;}

bool responseHandler::isCgi()
{
	try
	{
		t_cgi configs = conf.getCgiConf();
		std::string ext = getExt(path);
		std::vector<std::string>::iterator res = std::find(configs.extensions.begin(), configs.extensions.end(), ext);
		if (res != configs.extensions.end() && route.newRoot == configs.root)
		{
			if (configs.cgiAllowed)
			{
				if (access(path.c_str(), X_OK) == 0)
					return true;
				else
				{
					resp.respCode = 403;
					throw errorHandler(resp.respCodes[403]);
				}
			}
			else
			{
				resp.respCode = 403;
				throw errorHandler(resp.respCodes[403]);
			}
		}
		else if (res == configs.extensions.end() && route.newRoot != configs.root)
			return false;
		else
		{
			resp.respCode = 500;
			throw errorHandler(resp.respCodes[500]);
		}
	}
	catch(const std::exception& e)
	{
		return false;
	}
	return false;
}

void responseHandler::fillHeaders(std::string connection, std::string contLen)
{
	resp.headers["Server:"] = SRV;
	resp.headers["Date:"] = configUtils::getDateTime();
	resp.headers["Content-Length:"] = contLen;
	resp.headers["Connection:"] = connection;
	resp.headers["ETag:"] = eTag(route.newRoot + route.page);
	resp.headers["Accept-Ranges:"] = "bytes";

}

std::string responseHandler::getExt(std::string const &path)
{
	std::string ext;
	std::string::size_type fndPos = path.find('.');
	if (fndPos != std::string::npos)
		ext = path.substr(fndPos + 1);
	else
		return "none";
	return ext;
}

void responseHandler::getList()
{
	std::stringstream ss;

	ss << "<!DOCTYPE html>\n";
	ss << "<html><head><title>Index of " << request.route << " </title></head><body>\n";
	ss << "<h1>Index of " << request.route << "</h1>\n";
	ss << "<ul>\n";
	DIR *dir = opendir(route.newRoot.c_str());
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		std::string name(entry->d_name);
		if (name == ".." || name == ".")
			continue;
		ss << "<li><a href=" << request.route << "/" << name << ">" << name << "</a></li>\n";
	}
	ss << "</ul>\n";
	ss << "</body></html>\n";
	resp.body = ss.str();
	closedir(dir);
}

void responseHandler::uniqueName(std::string &flName)
{
	struct stat st;
	std::string path = route.newRoot;
	if (path[path.size() - 1] != '/')
		path += "/";
	std::string temp = path += flName;
	while (stat(temp.c_str(), &st) == 0)
	{
		size_t found = flName.find(".");
		if (found != std::string::npos)
		{
			std::string nwName = path.substr(0, found);
			std::string ext = getExt(flName);
			nwName + "_1" + "." + ext;
			temp.clear();
			temp = path + nwName;
			flName.clear();
			flName = nwName;
		}
		else
		{
			temp += "_1";
			flName += "_1";
		}
	}
}
