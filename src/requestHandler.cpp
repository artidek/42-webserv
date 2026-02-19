#include "../includes/configUtils.hpp"
#include "../includes/requestHandler.hpp"
#include "../includes/responseHandler.hpp"
#include <sys/socket.h>
#include <errno.h>
#include <iostream>
#include <unistd.h>

std::map<int, std::time_t>requestHandler::timeLog;
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

requestHandler::requestHandler(serverConfig const &conf) : _readLen(0), _host(conf), _contLen(0), _isHeader(false) {}

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
	_readLen = copy.getReadLen();
	_isHeader = copy.getIsheader();
	_readHeaders = copy.getReadHeader();
	buffChunks = copy.getBufChunks();
}

requestHandler &requestHandler::operator=(requestHandler const &copy)
{
	_host = copy.getConfig();
	_rawData = copy.getRawData();
	_endBody = copy.getEndBody();
	_tokens = copy.getTokens();
	_request = copy.getReqData();
	_contLen = copy.getContLen();
	_readLen = copy.getReadLen();
	_isHeader = copy.getIsheader();
	_readHeaders = copy.getReadHeader();
	buffChunks = copy.getBufChunks();
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
		if (readBytes > 0)
		{
			std::vector<char>chunk(buffer, buffer + readBytes);
			buffChunks.push_back(chunk);
			_readLen += readBytes;
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
	if (_readLen > _host.getHost().maxReqBody)
		throw errorHandler("Request Entity Too Large");
	checkTimeout(fd);
}

void requestHandler::setBodyEnd()
{
	std::string find = "boundary=";
	std::string::size_type found = _readHeaders.find(find);
	if (found != std::string::npos)
	{
		std::stringstream ss(_readHeaders.substr(found + find.size()));
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
		if (!token.empty() && token[token.size() - 1] == '\r' && token.size() > 1)
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

void requestHandler::addToTimeLog(int fd, std::time_t sec)
{
	std::map<int, std::time_t>::iterator res = timeLog.find(fd);
	if (res == timeLog.end())
		timeLog[fd] = sec;
}

void requestHandler::checkTimeout(int fd)
{
	std::time_t now = std::time(NULL);
	std::map<int, std::time_t>::iterator res = timeLog.find(fd);
	int reqTimeout = _host.getHost().hostTimeout;
	if (res != timeLog.end() && now - res->second >= reqTimeout)
	{
		timeLog.erase(fd);
		throw errorHandler("Request Timeout");
	}

}

bool requestHandler::requestComplete(void)
{
	if (!_isHeader)
		isHeaders();
	if (_isHeader)
	{
		if (_request.method == GET || _request.method  == HEAD || _request.method  == OPTIONS || _request.method == DELETE)
		{
			_rawData = combine();
			return true;
		}
		if (doneReading())
		{
			setBodyEnd();
			_rawData = combine();
			try
			{
				if (!_endBody.empty())
					fillReqBody();
				else
					fillReqBodyApp();
			}
			catch(const std::exception& e)
			{
				throw errorHandler("Bad Request");
			}
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
	std::string parseRoute = rawRoute;
	size_t queryPos = parseRoute.find("?");
	if (queryPos != std::string::npos)
	{
		_request.query = parseRoute.substr(queryPos + 1);
		parseRoute = parseRoute.substr(0, queryPos);
	}
	size_t lastSlash = parseRoute.rfind("/");
	std::string upToLastSlash;
	if (parseRoute.size() > 1 && lastSlash == 0)
		upToLastSlash = parseRoute.substr(0, 1);
	else if(lastSlash > 0)
		upToLastSlash = parseRoute.substr(0, lastSlash);
	if (!upToLastSlash.empty())
		getRoutePage(upToLastSlash, parseRoute, lastSlash);
	std::map<std::string, t_route>routes = _host.getRoutes();
	std::map<std::string, t_route>::iterator res = routes.find(parseRoute);
	if (res != routes.end())
	{
		if (!_request.page.empty())
			_request.page.clear();
		_request.route = parseRoute;
	}
}

void requestHandler::getRoutePage(std::string upToLastSlash, std::string parseRoute, size_t lastSlash)
{
	std::map<std::string, t_route>::iterator res;
	std::map<std::string, t_route>routes = _host.getRoutes();
	res = routes.find(upToLastSlash);
	if (res != routes.end())
	{
		_request.page = parseRoute.substr(lastSlash + 1);
		_request.route = upToLastSlash;
	}
}

std::ostream &operator<< (std::ostream &o, requestHandler const &req)
{
	t_request reqData = req.getReqData();
	o << "method: " << reqData.method << std::endl;
	o << "route: " << reqData.route << std::endl;
	o << "query: " << reqData.query << std::endl;
	o << "page: " << reqData.page << std::endl;
	o << "body content: " << reqData.body.content << std::endl;
	o << "body filename: " << reqData.body.fileName << std::endl;
	return o;
}

void requestHandler::setContLen()
{
	std::string contLenHead("Content-Length:");
	std::string::size_type found = _readHeaders.find(contLenHead);

	if (found != std::string::npos)
	{
		std::stringstream ss(_readHeaders.substr(found + contLenHead.size()));
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
	if (_readLen == static_cast<int>(_contLen))
		return true;
	return false;
}

std::string requestHandler::combine()
{
	size_t totalSize = 0;
    std::vector<std::vector<char> >::const_iterator it;
    for (it = buffChunks.begin(); it !=  buffChunks.end(); ++it)
        totalSize += it->size();
    std::string full;
    full.reserve(totalSize);
    for (it =  buffChunks.begin(); it !=  buffChunks.end(); ++it)
        full.append(&(*it)[0], it->size());
    return full;
}

void requestHandler::isHeaders()
{
	std::string combined = combine();
	size_t found = combined.find("\r\n\r\n");
	if (found != std::string::npos)
	{
		_readLen -= static_cast<int>(found) + 4;
		fillMethodRoute(combined);
		_readHeaders = combined.substr(0, found);
		setContLen();
		_isHeader = true;
	}
}

int requestHandler::getReadLen() const {return _readLen;}

bool requestHandler::getIsheader() const {return _isHeader;}

std::string const requestHandler::getReadHeader() const {return _readHeaders;}

std::vector<std::vector<char> > const requestHandler::getBufChunks() const {return buffChunks;}
