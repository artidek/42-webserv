#include "../includes/configUtils.hpp"
#include "../includes/requestHandler.hpp"
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
		return false;
	return true;
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
		buffer[readBytes] = 0;
		if (readBytes > 0)
			_rawData += buffer;
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
	try
	{
		checkTimeout(fd, configUtils::getTime());
	}
	catch(const std::exception& e)
	{
		throw errorHandler(std::string(e.what()));
	}
	if (errno == EAGAIN || errno == EWOULDBLOCK)
		std::cout << "socket drained succeffully \n";
}

void requestHandler::setBodyEnd(std::string token)
{
	std::string find = "boundary=";
	size_t found = token.find(find);
	if (found != std::string::npos)
	{
		std::stringstream ss(token.substr(found + find.size(), token.size() - 1));
		std::getline(ss, _endBody, ';');
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
		setBodyEnd(token);
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
	std::stringstream ss(value);
	std::string name;
	std::string find = "filename=";
	while (std::getline(ss, name, ';'))
	{
		size_t found = name.find(find);
		if (found != std::string::npos)
		{
			name = name.substr(found + find.size(), name.size());
			name = configUtils::trim(name, "\"");
			reqBody.fileName = name;
			break;
		}
	}
}

bool requestHandler::isBodyHeader(std::string &h, std::string &v, std::string const &token)
{
	std::stringstream ss(token);
	if (std::getline(ss, h, ':') && std::getline(ss, v))
	{
		if (h == "Content-Disposition" || h == "Content-Type")
			return true;
	}
	return false;
}

//Content-Disposition

t_reqBody requestHandler::fillReqBody(bool upload)
{
	t_reqBody res;
	std::string temp;
	std::string token;

	tokenize();
	if (upload)
	{
		if (_tokens.top().find(_endBody) != std::string::npos)
		{
			_tokens.pop();
			token = _tokens.top();
			while (token.find(_endBody) != std::string::npos)
			{
				temp += token;
				_tokens.pop();
				token = _tokens.top();
			}
			if (token[0] != '-')
				return res;
			_tokens.pop();
			while (_tokens.top().find("Content-Disposition:") != std::string::npos)
				_tokens.pop();
			if (!_tokens.empty())
			{
				getFileName(res, _tokens.top());
				res.content = temp;
			}
		}
	}
	else if (!upload)
	{
		token = _tokens.top();
		_tokens.pop();
		if (token != "\r")
			return res;
		if (token.size() != _contLen)
			return res;
		res.content = token;
	}
	return res;
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
		std::cout << "token " << token << std::endl;
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
	std::cout << "time diff " << sec - res->second << std::endl;
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

	_request.method = method;
	parseRoute(route);
	if (method == GET || method == HEAD || method == OPTIONS)
	{
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
		t_reqBody reqBd;
		if (!_endBody.empty() && _contLen > 0)
			reqBd =fillReqBody(true);
		else if (_contLen > 0)
			reqBd =fillReqBody(true);
		if (!reqBd.empty())
		{
			_request.body = reqBd;
			return true;
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
	std::cout << _request.route << " route\n";
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
		_request.route += "/";
		_request.route += tokens[i];
	}
}

std::ostream &operator<< (std::ostream &o, requestHandler const &req)
{
	o << "method: " << req.getReqData().method << std::endl;
	o << "route: " << req.getReqData().route << std::endl;
	o << "query: " << req.getReqData().query << std::endl;
	o << "page: " << req.getReqData().page << std::endl;
	o << "path_info: " << req.getReqData().path_info<< std::endl;
	o << "body content: " << req.getReqData().body.content << std::endl;
	o << "body filename: " << req.getReqData().body.fileName << std::endl;
	return o;
}

void requestHandler::setContLen()
{
	std::string contLen("Content-Length:");
	size_t found = _rawData.find(contLen);

	if (found != std::string::npos)
	{
		std::stringstream ss(_rawData.substr(found + contLen.size(), _rawData.size() - 1));
		std::string contLen;
		if (std::getline(ss, contLen, '\r'))
		{
			ss.clear();
			ss << contLen;
			ss >> _contLen;
		}
	}
}

int const requestHandler::getContLen(void) const {return _contLen;}
