#include "../includes/configUtils.hpp"
#include "../includes/requestHandler.hpp"
#include "../includes/responseHandler.hpp"
#include <sys/socket.h>
#include <errno.h>
#include <iostream>
#include <unistd.h>
#include <cstring>

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

requestHandler::requestHandler(std::map<std::string, serverConfig> const &hosts) : _readLen(0), _contLen(0), _totalSize(0), accumulated(0), configs(hosts), _isHeader(false) {}

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
	_totalSize = copy.getTotalSize();
	configs = copy.getConfigs();
	accumulated = getAccum();
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
	_totalSize = copy.getTotalSize();
	configs = copy.getConfigs();
	accumulated = copy.getAccum();
	return *this;
}

std::string const &requestHandler::getRawData(void) const { return _rawData; }

void requestHandler::read(int const &fd)
{
	int readBytes = 0;
	if (buffChunks.empty())
		buffChunks.resize(BUFFER_SIZE);
	while(true)
	{
		// if (static_cast<int>(buffChunks.size()) - _readLen < BUFFER_SIZE)
		// 		buffChunks.resize(buffChunks.size() + BUFFER_SIZE);
		if (buffChunks.size() - accumulated > 0)
		{
			readBytes = recv(fd, &buffChunks[accumulated], buffChunks.size() - accumulated, 0);
			if (readBytes > 0)
			{
				_readLen += readBytes;
				accumulated += readBytes;
				if (_totalSize == 0)
					_totalSize += _readLen;
				if (!_isHeader)
					extractHeaders();
				if (_isHeader)
				{
					if (_contLen > 0)
					{
						if (buffChunks.size() < _totalSize)
							buffChunks.resize(_totalSize);
					}
				}
		}
			else if (readBytes == 0)
				throw errorHandler("Bad request read close");
			else
			{	
				if (errno == EINTR)
					continue;
				else if (errno == EAGAIN || errno == EWOULDBLOCK)
					break;
				else
					throw errorHandler("Bad request read fail");
			}
		}
		else
			break;
	}
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
	 // Boundaries
	std::string startBoundary = "--" + _endBody;
	std::string endBoundary = "--" + _endBody + "--";
	// Find the start boundary
	size_t startPos = _rawData.find(startBoundary);
	if (startPos == std::string::npos)
		throw errorHandler("Bad request fillreqbody");
	 size_t skipPos = startPos + startBoundary.size();
	 // Find end of headers (\r\n\r\n)
	size_t startBody = _rawData.find("\r\n\r\n", skipPos);
	if (startBody == std::string::npos)
        throw errorHandler("Bad request: headers missing");
	 // Extract filename safely without copying entire headers
    size_t filePos = _rawData.find("filename=", skipPos);
    if (filePos != std::string::npos && filePos < startBody)
    {
        size_t fileEndPos = _rawData.find("\n", filePos + 9);
        if (fileEndPos == std::string::npos)
            fileEndPos = startBody;
        _request.body.fileName = _rawData.substr(filePos + 9, fileEndPos - (filePos + 9));
         _request.body.fileName = configUtils::trim( _request.body.fileName, " \"\r");
    }
	 // Calculate body start position
	size_t bodyStart = startBody + 4;
	// Find end boundary directly in the raw data
    size_t stopPos = _rawData.find(endBoundary, bodyStart);
    if (stopPos == std::string::npos)
	{
		throw errorHandler("Bad request: end boundary missing");
	}
	size_t contentLength = stopPos - bodyStart;
	//extract body content
	if (contentLength > 0)
		_request.body.content.assign(_rawData, bodyStart, contentLength);
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
}

void requestHandler::parse(void)
{
	if (_rawData.empty())
		throw errorHandler("Bad request parse");
	std::stringstream ss(_readHeaders);
	std::string headerName;
	std::string headerVal;
	while (std::getline(ss, headerName, ':') && std::getline(ss, headerVal))
	{
		headerName = configUtils::trim(headerName, " :");
		headerVal = configUtils::trim(headerVal, " \r\n;");
		fillHeader(headerName, headerVal);
	}
}

t_request const requestHandler::getReqData(void) const { return _request; }

bool requestHandler::requestComplete(void)
{
	if (_isHeader)
	{
		if (_request.method == GET || _request.method  == HEAD || _request.method  == OPTIONS || _request.method == DELETE)
		{
			_rawData = combine();
			fillMethodRoute(_readHeaders);
			return true;
		}
		if (doneReading())
		{
			try
			{
				setBodyEnd();
				_rawData = combine();
				if (!_endBody.empty())
					fillReqBody();
				else
					fillReqBodyApp();
			}
			catch(const std::exception& e)
			{
				throw errorHandler("Bad Request complete");
			}
			fillMethodRoute(_readHeaders);
			return true;
		}
	}
	return false;
}

serverConfig const requestHandler::getConfig(void) const {return _host;}

std::string const requestHandler::getEndBody(void) const {return _endBody;}

std::stack<std::string> const requestHandler::getTokens(void) const {return _tokens;}

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
	if (_readLen > _host.getHost().maxReqBody)
		throw errorHandler("Request Entity Too Large");
	if (_readLen == static_cast<int>(_contLen))
		return true;
	return false;
}

std::string requestHandler::combine()
{
	std::string full;
	full.append(&buffChunks[0], accumulated);
    return full;
}

void requestHandler::extractHeaders()
{
	std::string combined = combine();
	size_t found = combined.find("\r\n\r\n");
	if (found != std::string::npos)
	{
		_readHeaders = combined.substr(0, found);
		_readLen -= static_cast<int>(found) + 4;
		setContLen();
		if (_contLen > 0)
			_totalSize += _contLen;
		_isHeader = true;
	}
}

int requestHandler::getReadLen() const {return _readLen;}

bool requestHandler::getIsheader() const {return _isHeader;}

std::string const requestHandler::getReadHeader() const {return _readHeaders;}

std::vector<char> const requestHandler::getBufChunks() const {return buffChunks;}

size_t requestHandler::getTotalSize() const {return _totalSize;}

std::map<std::string, serverConfig> requestHandler::getConfigs() const {return configs;}

bool requestHandler::headersOk()
{
	if (_isHeader)
		return true;
	return false;
}

t_host requestHandler::setHost()
{
	std::string hostHeader = "Host:";
	size_t pos = _readHeaders.find(hostHeader);
	std::stringstream ss(_readHeaders.data() + pos + hostHeader.size() + 1);
	std::string hostName;
	std::getline(ss, hostName, '\n');
	if (!hostName.empty())
	{
		configUtils::trim(hostName," \r\n");
		size_t found = hostName.find(":");
		hostName = hostName.substr(0, found);
		if (configs.find(hostName) != configs.end())
			_host = configs[hostName];
		else
			throw errorHandler("Bad Request host");
	}
	return _host.getHost();
}

size_t requestHandler::getAccum() const {return accumulated;}
