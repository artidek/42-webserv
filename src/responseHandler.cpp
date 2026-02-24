#include "../includes/requestHandler.hpp"
#include "../includes/responseHandler.hpp"
#include "../includes/configUtils.hpp"
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <dirent.h>
#include <cstdio>

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
	respCodes[400] = "Bad request";
	respCodes[401] = "Unauthorized";
	respCodes[403] = "Forbidden";
	respCodes[404] = "Not Found";
	respCodes[405] = "Method Not Allowed";
	respCodes[408] = "Request Timeout";
	respCodes[413] = "Request Entity Too Large";
	respCodes[415] = "Unsupported Media Type";
	respCodes[500] = "Internal Server Error";
	respCodes[501] = "Not Implemented";
	respCodes[502] = "Bad Gateway";
	respCodes[503] = "Service Unavailable";
}

responseHandler::responseHandler(requestHandler const &req) : size(0), total(0), request(req) {

	runMethod[GET] = &responseHandler::runGet;
	runMethod[POST] = &responseHandler::runPost;
	runMethod[HEAD] = &responseHandler::runHead;
	runMethod[DELETE] = &responseHandler::runDelete;
	emptyBody = false;
	sendComplete = false;
	cgi = false;
}

responseHandler::responseHandler() : sendComplete(false), size(0), total(0)
{

}

responseHandler::responseHandler(responseHandler const & copy)
{
	size = copy.getSize();
	total = copy.getTotal();
	buffer = copy.getBuffer();
	sendComplete = copy.getComplete();
	resp.respCode = copy.getResponceData().respCode;
}

responseHandler &responseHandler::operator=(responseHandler const &copy)
{
	size = copy.getSize();
	total = copy.getTotal();
	buffer = copy.getBuffer();
	sendComplete = copy.getComplete();
	resp.respCode = copy.getResponceData().respCode;
	return *this;
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

void responseHandler::allowedMethod()
{
	t_route rt;
	try
	{
		rt = conf.getRoute(request.getReqData().route);
	}
	catch(const std::exception& e)
	{
		resp.respCode = 404;
		throw errorHandler(resp.respCodes[404]);
	}
	std::string method = request.getReqData().method;
	std::vector <std::string>::iterator res = std::find(rt.methods.begin(), rt.methods.end(), method);
	if (res == rt.methods.end())
	{
		resp.respCode = 405;
		throw errorHandler(resp.respCodes[405]);
	}
}

void responseHandler::isRoute(t_route &route)
{
	try
	{
		route = conf.getRoute(request.getReqData().route);
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
		t_location location = conf.getLocation(route.newRoot);
		if (checkNone(path))
		{
			if (location.enableListing)
				getList();
			else
			{
				resp.respCode = 403;
				throw errorHandler(resp.respCodes[403]);
			}
		}
		else
		{
			if (access(path.c_str(), R_OK) == 0)
			{
				if (resp.respCode == 301 || resp.respCode == 302)
					resp.headers["Location:"] = conf.getRoute(request.getReqData().route).redirect;
				else
					fillResponseBody(path);
			}
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
		if (checkNone(path))
		{
			if (conf.getLocation(route.newRoot).enableUpload)
			{
				std::string ext;
				std::map<std::string, std::string>headers = request.getReqData().headers;
				std::map<std::string, std::string>::iterator res =  headers.find("Content-Type");
				if ( res != headers.end())
				{
					std::string type = configUtils::trim(res->second, " \r\n");
					size_t found = type.find(";");
					if (found != std::string::npos)
						type = type.substr(0, found);
					if (type != "multipart/form-data" && !conf.checkMimeTypes(type, ext))
						throw errorHandler("Unsupported Media Type");
				}
				else
					throw errorHandler("Unsupported Media Type");
				std::string filename = request.getReqData().body.fileName;
				if (filename.empty())
					filename = "uploaded_file" + ext;
				filename = configUtils::trim(filename, "\"");
				uniqueName(filename);
				std::string path = route.newRoot;
				if (path[path.size() - 1] != '/')
				path += '/';
				path += filename;
				if (allowedExt(filename, true))
				{
					std::fstream file(path.c_str(), std::ios::out | std::ios::binary);
					file.write(request.getReqData().body.content.c_str(), request.getReqData().body.content.size());
					file.close();
					std::stringstream size;
					size << resp.body.size();
					fillHeaders("keep-alive", size.str());
					resp.headers["Location:"] = configUtils::buildPath(request.getReqData().route, filename);
					resp.respCode = 201;
				}
			}
			else
			{
				resp.respCode = 403;
				throw errorHandler(resp.respCodes[403]);
			}
		}
		
	}
	catch(const std::exception& e)
	{
		throw errorHandler(std::string(e.what()));
	}
}

void responseHandler::runHead(void)
{
	if (checkNone(path) || access(path.c_str(), R_OK) != 0)
	{
		resp.respCode = 404;
		throw errorHandler(resp.respCodes[404]);
	}
	std::fstream file(path.c_str());
	std::stringstream ss;
	ss << file.rdbuf();
	std::stringstream size;
	size << ss.str().size();
	fillHeaders("close", size.str());
}

void responseHandler::runDelete(void)
{
	try
	{
		if (request.getReqData().page.empty())
		{
			resp.respCode = 404;
			throw errorHandler(resp.respCodes[404]);
		}
		if (std::remove(path.c_str()) != 0)
		{
			resp.respCode = 403;
			throw errorHandler(resp.respCodes[403]);
		}
		fillResponseBody("etc/error/success.html");
	}
	catch(const std::exception& e)
	{
		throw errorHandler(std::string(e.what()));
	}
}

void responseHandler::isMethod(std::string &mtd)
{
	std::string m = request.getReqData().method;
	if (m == OPTIONS)
	{
		std::map<std::string, std::string> headers = request.getReqData().headers;
		m = headers["Access-Control-Request-Method"];
		if (m != GET && m != POST && m != DELETE && m != HEAD)
		{
			resp.respCode = 501;
			throw errorHandler(resp.respCodes[501]);
		}
		else
			mtd = m;
	}
	else if (m == GET || m == POST || m == DELETE || m == HEAD)
		mtd = m;
	else
	{
		resp.respCode = 501;
		throw errorHandler(resp.respCodes[501]);
	}
}

void responseHandler::createResponce(std::vector<std::string> const &envp)
{
	std::string method;
	try
	{
		conf = request.getConfig();
		isMethod(method);
		isRoute(route);
		allowedMethod();
		if (route.newRoot == "none" && request.getReqData().page.empty())
		{
			resp.respCode = 500;
			throw errorHandler(resp.respCodes[500]);
		}
		else
		{
			path = configUtils::buildPath(route.newRoot,route.page);
			if (!request.getReqData().page.empty())
				path = configUtils::buildPath(route.newRoot, request.getReqData().page);
		}
		std::stringstream ss(route.response);
		ss >> resp.respCode;
		if (isCgi())
			runCgi(envp);
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

void responseHandler::fillSendBuffer()
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
	size = buffer.size();
}

void responseHandler::sendToClient(int const &fd)
{
	while (total < size)
	{
		int writeBytes = send(fd, buffer.data() + total, size - total, 0);
		if (writeBytes < 0)
		{
			if (errno == EINTR) continue;
			else if (errno == EAGAIN || errno == EWOULDBLOCK)
				return;
			else
				throw errorHandler("Send failed");
		}
		total += writeBytes;
	}
	if (size == total)
		sendComplete = true;
}

void responseHandler::sendResponse(int const &fd)
{
	if (!cgi)
		fillSendBuffer();
	try
	{
		sendToClient(fd);
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
	fillResponseBody(conf.getErrorPage(respCode));
	std::stringstream ss;
	ss << resp.body.size();
	std::string sz;
	ss >> sz;
	resp.headers["Content-Length:"] = sz;
	fillSendBuffer();
	sendToClient(fd);
}

int  responseHandler::getRespCode(void) const {return resp.respCode;}

bool responseHandler::responseComplete(void) {return sendComplete;}

bool responseHandler::isCgi()
{
	t_cgi cgiConf = conf.getCgiConf();
	//add slash to the end if not in root or new_root
	if (cgiConf.root != route.newRoot)
		return false;
	return true;
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
	ss << "<html><head><title>Index of " << request.getReqData().route << " </title></head><body>\n";
	ss << "<h1>Index of " << request.getReqData().route << "</h1>\n";
	ss << "<ul>\n";
	std::string path = route.newRoot;
	DIR *dir = opendir(path.c_str());
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		path = route.newRoot;
		std::string name(entry->d_name);
		if (name == ".." || name == ".")
			continue;
		struct stat info;
		if (path[path.size() - 1] == '/')
			path += "/";
		path += name;
		if (isDir(&info, path))
			continue;
		if (S_ISREG(info.st_mode))
		{
			if (allowedExt(name, false))
				ss << "<li>" << name << "</li>\n";
		}
	}
	ss << "</ul>\n";
	ss << "</body></html>\n";
	resp.body = ss.str();
	closedir(dir);
}

void responseHandler::uniqueName(std::string &flName)
{
	struct stat st;
	int count = 0;
	std::string path = route.newRoot;
	if (path[path.size() - 1] != '/')
		path += "/";
	std::string temp = path + flName;
	while (stat(temp.c_str(), &st) == 0)
	{
		count++;
		std::stringstream ss;
		ss << count;
		size_t found = flName.rfind(".");
		if (found != std::string::npos)
		{
			std::string nwName = flName.substr(0, found);
			std::string ext = getExt(flName);
			nwName += "_" + ss.str() + "." + ext;
			temp.clear();
			temp = path + nwName;
			flName.clear();
			flName = nwName;
		}
		else
		{
			temp += "_" + ss.str();
			flName += "_" + ss.str();
		}
	}
}

int responseHandler::findRespCode(std::string const &err)
{
	std::map<int, std::string>::iterator begin = resp.respCodes.begin();
	std::map<int, std::string>::iterator end = resp.respCodes.end();
	for (; begin != end; ++begin)
	{
		if (begin->second == err)
			return begin->first;
	}
	return 0;
}

size_t responseHandler::getSize() const {return size;}

size_t responseHandler::getTotal() const {return total;}

std::string const responseHandler::getBuffer() const {return buffer;}

bool responseHandler::getComplete() const {return sendComplete;}

void responseHandler::runCgi(std::vector<std::string> env)
{
	(void)env;
	//cgi execution goes here
}

bool responseHandler::checkNone(std::string const &path)
{
	std::string none = path.substr(path.size() - 4);
	if (none != "none")
		return false;
	return true;
}

bool responseHandler::isDir(struct stat *info, std::string const &fullPath)
{
	if(stat(fullPath.c_str(), info) == -1)
		return true;
	if (S_ISDIR(info->st_mode))
		return true;
	return false;
}

bool responseHandler::allowedExt(std::string const &name, bool upload)
{
	size_t found = name.find_last_of(".");
	std::string ext;
	if (found != std::string::npos)
		ext = name.substr(found + 1);
	else
		return false;
	if (!upload)
	{
		std::vector<std::string> exts = conf.getLocation(route.newRoot).listExt;
		std::vector<std::string>::iterator res = find(exts.begin(), exts.end(), ext);
		if (res != exts.end())
			return true;
	}
	else
	{
		std::vector<std::string> exts = conf.getLocation(route.newRoot).uploadExt;
		std::vector<std::string>::iterator res = find(exts.begin(), exts.end(), ext);
		if (res == exts.end())
		{
			resp.respCode = 403;
			throw errorHandler(resp.respCodes[403]);
		}
		else
			return true;
	}
	return false;
	
}
