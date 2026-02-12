#include "../includes/configUtils.hpp"
#include "../includes/requestHandler.hpp"
#include "../includes/responseHandler.hpp"
#include <sys/socket.h>
#include <errno.h>
#include <iostream>
#include <unistd.h>

std::map<int, double>requestHandler::timeLog;
std::map<std::string, std::string> requestHandler::_headers = initHeaders();

std::map<std::string, std::string> requestHandler::initHeaders(void)
{
	std::map<std::string, std::string> res;
	res["Accept"] = "";
	res["User-Agent"] = "";
	res["referer"] = "";
	res["Authorization"] = "";
	res["From"] = "";
	res["If-Modified-Since"] = "";
	res["Content-Type"] = "";
	res["Content-Length"] = "";
	res["Access-Control-Request-Method"] = "";
	return res;
}

bool t_reqBody::empty(void)
{
	if (fileName.empty() && content.empty())
		return true;
	return false;
}

requestHandler::requestHandler(serverConfig const &conf) : _host(conf), _contLen(0) {}

requestHandler::requestHandler(void) {}

requestHandler::~requestHandler(void) {}

requestHandler::requestHandler(requestHandler const &copy)
{
	_host = copy.getConfig();
	_rawData = copy.getRawData();
	_endBody = copy.getEndBody();
	_tokens = copy.getTokens();
	_request = copy.getReqData();
	_contLen = copy.getContLen();
}

requestHandler &requestHandler::operator=(requestHandler const &copy)
{
	_host = copy.getConfig();
	_rawData = copy.getRawData();
	_endBody = copy.getEndBody();
	_tokens = copy.getTokens();
	_request = copy.getReqData();
	_contLen = copy.getContLen();
	return *this;
}

std::string const &requestHandler::getRawData(void) const { return _rawData; }

void requestHandler::read(int const &fd)
{
	int readBytes = 0;

	while(true)
	{
		char buffer[BUFFER_SIZE + 1];
		readBytes = recv(fd, buffer, sizeof(buffer),0);
		//buffer[readBytes] = 0;
		if (readBytes > 0)
			_rawData.append(buffer, readBytes);
		else if (readBytes == 0)
			throw errorHandler("Client closed connection");
		else
		{
			if (errno == EINTR)
				continue;
			else if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			else
				throw errorHandler("Reading error");
		}
	}
}

void requestHandler::setBodyEnd(std::string token)
{
	std::string find = "boundary=";
	std::string::size_type found = token.find(find);
	if (found != std::string::npos)
	{
		std::stringstream ss(token.substr(found + find.size()));
		std::getline(ss, _endBody);
		if (_endBody[_endBody.size() - 1] == '\r')
			_endBody.resize(_endBody.size() - 1);
	}
}

void requestHandler::tokenize(void)
{
	std::stringstream ss(_rawData);
	std::string token;
	while (std::getline(ss, token, '\n'))
	{
		if (token[token.size() - 1] == '\r' && token.size() > 1)
			token = token.substr(0, token.size() - 1);
		if (!token.empty())
			_tokens.push(token);
	}
}

void requestHandler::fillHeader(std::string headerProp, std::string headerVal)
{
	std::map<std::string, std::string>::iterator res = _headers.find(headerProp);
	if (res != _headers.end())
		_request.headers[headerProp] = headerVal;
}

void requestHandler::fillMethodRoute(std::string headerProp)
{
	std::stringstream ss(headerProp);
	std::string method;
	std::string rawRoute;
	if (std::getline(ss, method, ' ') && std::getline(ss, rawRoute, ' '))
	{
		_request.method = method;
		parseRoute(rawRoute);
	}
}

void requestHandler::getFileName(t_reqBody &reqBody, std::string value)
{
	std::string property = "filename=";
	std::string trimmed = configUtils::trim(value, "\r\n");
	size_t pos = trimmed.find(property);
	if (pos != std::string::npos)
		reqBody.fileName = trimmed.substr(pos + property.size());
}

bool requestHandler::isBodyHeader(std::string &h, std::string &v, std::string const &token)
{
	std::stringstream ss(token);
	if (std::getline(ss, h, ':') && std::getline(ss, v))
	{
		if (v[v.size() - 1] == '\r')
			v = v.substr(0, v.size() - 1);
		if (h == "Content-Disposition" || h == "Content-Type")
			return true;
	}
	return false;
}

//Content-Disposition

void requestHandler::fillReqBody()
{
	std::string startBoundary = "--" + _endBody;
	std::string endBoundary = "--" + _endBody + "--";
	size_t startPos = _rawData.find(startBoundary);
	if (startPos == std::string::npos)
		throw errorHandler("No starting boundary");
	std::string dataAfterBoundary = _rawData.substr(startPos + startBoundary.size());
    std::stringstream ss(dataAfterBoundary);
	std::string token;
	//std::getline(ss, token, '\n');
	// Skip initial \r\n after boundary
    if (ss.peek() == '\r') ss.get();
    if (ss.peek() == '\n') ss.get();
	//Skip headers and extract filename
	 while (std::getline(ss, token) && !token.empty() && token != "\r") {getFileName(_request.body, token);}
	//Read the rest of the body including end boundary
	std::string restOfReq((std::istreambuf_iterator<char>(ss)),std::istreambuf_iterator<char>());
	size_t stopPos = restOfReq.find(endBoundary);
	if (stopPos == std::string::npos)
		throw errorHandler("No ending boundary");
	//Extract body content	
	_request.body.content = restOfReq.substr(0, stopPos);
}

void requestHandler::fillReqBodyApp()
{
	std::stringstream ss(_rawData);
	std::string token;
	while (std::getline(ss, token) && !token.empty() && token != "\r") {}
	 std::string restOfReq((std::istreambuf_iterator<char>(ss)),std::istreambuf_iterator<char>());
	 if (restOfReq.size() < _contLen)
	 	throw errorHandler("Not fully read");
	_request.body.content = restOfReq;
	_request.body.fileName = "uploadded_file";
}

void requestHandler::parse(void)
{
	if (_rawData.empty())
		throw errorHandler("Bad request");
	tokenize();
	std::string token;
	while (!_tokens.empty())
	{
		token = _tokens.top();
		_tokens.pop();
		std::stringstream temp(token);
		std::string headerProp;
		std::string headerVal;
		if (std::getline(temp, headerProp, ':') && std::getline(temp, headerVal))
			fillHeader(headerProp, headerVal);
	}
}

t_request const requestHandler::getReqData(void) const { return _request; }

void requestHandler::addToTimeLog(int fd, double sec)
{
	std::map<int, double>::iterator res = timeLog.find(fd);
	if (res == timeLog.end())
		timeLog[fd] = sec;
}

void requestHandler::checkTimeout(int fd, double sec)
{
	std::map<int, double>::iterator res = timeLog.find(fd);
	int reqTimeout = _host.getHost().hostTimeout;
	std::cout << "timelog " << std::fixed << res->second << " time left sec " << std::fixed << sec << std::endl;
	if (sec - res->second >= reqTimeout)
	{
		timeLog.erase(fd);
		throw errorHandler("Request Timeout");
	}

}

bool requestHandler::requestComplete(void)
{
	std::stringstream ss(_rawData.c_str());
	std::string method;
	std::string route;
	std::string line;
	std::getline(ss, method, ' ');
	std::getline(ss, route, ' ');

	if (method == GET || method == HEAD || method == OPTIONS)
	{
		_request.method = method;
		parseRoute(route);
		while (std::getline(ss, line, '\n'))
		{
			if (line == "\r")
				return true;
		}
	}
	else
	{
		setBodyEnd(_rawData);
		setContLen();
		try
		{
			if (!_endBody.empty() && doneReading())
			{
				fillReqBody();
				_request.method = method;
				parseRoute(route);
			}
			else if (doneReading())
			{
				_request.method = method;
				parseRoute(route);
				fillReqBodyApp();
			}
			if (!_request.body.empty())
				return true;
		}
		catch(const std::exception& e)
		{
			std::cout << e.what() << " it comes from request complete\n" << std::endl;
			return false;
		}
	}
	return false;
}

serverConfig const requestHandler::getConfig(void) const {return _host;}

std::string const requestHandler::getEndBody(void) const {return _endBody;}

std::stack<std::string> const requestHandler::getTokens(void) const {return _tokens;}

void requestHandler::removeFromTimeLog(int const &fd) {timeLog.erase(fd);}

void requestHandler::parseRoute(std::string const &rawRoute)
{
	std::vector<std::string> tokens;
	std::string token;
	std::stringstream ss;
	size_t found = rawRoute.find("?");
	if (found != std::string::npos)
	{
		ss << rawRoute.substr(0, found);
		_request.query = rawRoute.substr(found + 1);
	}
	else
		ss << rawRoute;
	while (std::getline(ss, token, '/'))
	{
		if (token.find(".") != std::string::npos)
		{
			_request.page = token;
			extractPathInfo(ss);
			break;
		}
		if (!token.empty())
			tokens.push_back(token);
	}
	buildRoute(tokens);
}

void requestHandler::extractPathInfo(std::stringstream const &ss)
{
	std::string pathInfo = "/";
	if (!ss.eof())
	{
		pathInfo += ss.str();
		_request.path_info = pathInfo;
	}
}

void requestHandler::buildRoute(std::vector<std::string> const &tokens)
{
	if (tokens.empty())
		_request.route = "/";
	for (size_t i = 0; i < tokens.size(); i++)
	{
		_request.route.append("/", 1);
		_request.route.append(tokens[i]);
	}
}

std::ostream &operator<< (std::ostream &o, requestHandler const &req)
{
	t_request reqData = req.getReqData();
	o << "method: " << reqData.method << std::endl;
	o << "route: " << reqData.route << std::endl;
	o << "query: " << reqData.query << std::endl;
	o << "page: " << reqData.page << std::endl;
	o << "path_info: " << reqData.path_info<< std::endl;
	o << "body content: " << reqData.body.content << std::endl;
	o << "body filename: " << reqData.body.fileName << std::endl;
	return o;
}

void requestHandler::setContLen()
{
	std::string contLenHead("Content-Length:");
	std::string::size_type found = _rawData.find(contLenHead);

	if (found != std::string::npos)
	{
		std::stringstream ss(_rawData.substr(found + contLenHead.size()));
		std::string contLen;
		if (std::getline(ss, contLen, '\n'))
		{
			std::stringstream toInt(contLen);
			toInt << contLen;
			toInt >> _contLen;
		}
	}
}

size_t requestHandler::getContLen(void) const {return _contLen;}

bool requestHandler::doneReading()
{
	std::stringstream ss(_rawData);
	std::string token;
	while (std::getline(ss, token) && !token.empty() && token != "\r") {}
	 std::string restOfReq((std::istreambuf_iterator<char>(ss)),std::istreambuf_iterator<char>());
	if (restOfReq.size() == _contLen)
		return true;
	return false;
}