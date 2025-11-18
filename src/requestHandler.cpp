#include "../includes/requestHandler.hpp"
#include <sys/socket.h>
#include <errno.h>
#include <iostream>
#include "../includes/configUtils.hpp"

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

bool t_request::empty(void)
{
	if (body.empty() && method.empty() && route.empty() && headers.empty())
		return false;
	return true;
}

requestHandler::requestHandler(serverConfig const &copy) : _host(copy) {}

requestHandler::~requestHandler(void) {}

std::string const &requestHandler::getRawData(void) const { return _rawData; }

void requestHandler::read(int const &fd)
{
	int readSize = _host.getHost().maxReqBody + BUFFER_SIZE;
	int readBytes = 0;
	int totalRead = 0;
	
	while(true)
	{
		char buffer[BUFFER_SIZE + 1];
		readBytes = recv(fd, buffer, BUFFER_SIZE, 0);
		if (readBytes > 0 && totalRead <= readSize)
		{
			_rawData += buffer;
			totalRead += readBytes;
		}
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
	std::stringstream ss(token);
	std::string header;
	std::string value;
	if (std::getline(ss, header, ':') && std::getline(ss, value))
	{
		if (header == "Content-Type")
		{
			std::stringstream temp(value);
			std::string tempVal;
			while (std::getline(temp, tempVal, ';'))
			{
				std::string find = "boundary=";
				size_t found = tempVal.find(find);
				if (found != std::string::npos)
				{
					_endBody = tempVal.substr(found + find.size(), tempVal.size() - 1);
					_endBody = configUtils::trim(_endBody, "-");
				}
			}
		}
	}
}

void requestHandler::tokenize(void)
{
	std::stringstream ss(_rawData);
	std::string token;
	while (std::getline(ss, token, '\n'))
	{
		if (token[token.size() - 1] == '\r')
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
	std::string route;
	if (std::getline(ss, method, ' ') && std::getline(ss, route, ' '))
	{
		_request.method = method;
		_request.route = route;
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
			name = name.substr(found + find.size() + 1, name.size() - 1);
			reqBody.fileName = name;
			break;
		}
	}
}

t_reqBody requestHandler::fillReqBody(void)
{
	t_reqBody res;
	std::string token;

	while (!_tokens.empty())
	{
		token = _tokens.top();
		_tokens.pop();
		if (token.find(_endBody) != std::string::npos)
			break;
		else 
		{
			std::stringstream ss(token);
			std::string h;
			std::string v;
			if (std::getline(ss, h, ':') && std::getline(ss, v))
			{
				if (h == "Content-Disposition")
					getFileName(res, v);
			}
			else
				res.content += v;
		}
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
		_tokens.pop();
		std::stringstream temp(token);
		std::string headerProp;
		std::string headerVal;
		if (std::getline(temp, headerProp, ':') && std::getline(temp, headerVal))
			fillHeader(headerProp, headerVal);
		else if(headerVal.empty() && !_tokens.empty())
		{
			t_reqBody reqBody = fillReqBody();
			_request.body = reqBody;
		}
		else
			fillMethodRoute(headerProp);
	}
}

t_request requestHandler::getReqData(void) const { return _request; }