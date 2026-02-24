#include "../includes/serverConfig.hpp"
#include <sys/types.h>
#include <dirent.h>
#include <algorithm>
#include <unistd.h>
#include "../includes/configUtils.hpp"
#include <iostream>

const std::map<std::string, std::string> serverConfig::env = serverConfig::makeEnv();

bool s_host::empty(void)
{
	if (addr.empty() && ports.empty())
		return true;
	return false;
}


bool s_route::empty(void)
{
	if (newRoot.empty() && page.empty() && response.empty())
		return true;
	return false;
}

bool s_location::empty(void)
{
	if (listExt.empty() || uploadExt.empty())
		return true;
	return false;
}

bool s_cgi::empty(void)
{
	if (defaultCgi.empty())
		return true;
	return false;
}

s_cgi& s_cgi::operator=(s_cgi const &copy)
{
	if (this != &copy)
	{
		this->cgiAllowed = copy.cgiAllowed;
		this->defaultCgi = copy.defaultCgi;
		this->root = copy.root;
		this->extensions.clear();
		for (size_t i = 0; i < copy.extensions.size(); i++)
			this->extensions.push_back(copy.extensions[i]);
	}
	return *this;
}

std::map<std::string, std::string> serverConfig::makeEnv(void)
{
	std::map<std::string, std::string> m;
	m["Accept"] = "HTTP_ACCEPT";
	m["User-Agent"] = "HTTP_USER_AGENT";
	m["Referer"] = "HTTP_REFERER";
	m["Authorization"] = "HTTP_AUTHORIZATION";
	m["From"] = "HTTP_FROM";
	m["If-Modified-Since"] = "HTTP_IF_MODIFIED_SINCE";
	m["Content-Type"] = "CONTENT_TYPE";
	m["Content-Length"] = "CONTENT_LENGTH";
	return m;
}

serverConfig::serverConfig(void) {
	errorPages[400] = "etc/error/400.html";
	errorPages[401] = "etc/error/401.html";
	errorPages[402] = "etc/error/402.html";
	errorPages[403] = "etc/error/403.html";
	errorPages[404] = "etc/error/404.html";
	errorPages[405] = "etc/error/405.html";
	errorPages[408] = "etc/error/408.html";
	errorPages[415] = "etc/error/415.html";
	errorPages[413] = "etc/error/413.html";
	errorPages[500] = "etc/error/500.html";
	errorPages[501] = "etc/error/501.html";
	errorPages[502] = "etc/error/502.html";
	cgiConf.cgiAllowed = "off";
	cgiConf.root = "etc/cgi_bin";
	cgiConf.defaultCgi = "none";
	cgiConf.extensions.push_back("php");
	cgiConf.extensions.push_back("py");
	setMimeTypes();
}

serverConfig::serverConfig(serverConfig const &copy) : mimeTypes(copy.getMimeTypes()),locations(copy.getLocations()), routes(copy.getRoutes()), errorPages(copy.getErrorPages()), host(copy.getHost()), cgiConf(copy.getCgiConf()){}

serverConfig& serverConfig::operator=(serverConfig const &copy)
{
	if (this != &copy)
	{
		locations = copy.getLocations();
		routes = copy.getRoutes();
		errorPages = copy.getErrorPages();
		host = copy.getHost();
		cgiConf = copy.getCgiConf();
		mimeTypes = copy.getMimeTypes();
	}
	
	return *this;
}

void serverConfig::setMimeTypes()
{
	mimeTypes["text/plain"] = ".txt";
    mimeTypes["text/html"] = ".html";
    mimeTypes["text/css"] = ".css";
    mimeTypes["text/csv"] = ".csv";
    mimeTypes["text/xml"] = ".xml";

    // Images
    mimeTypes["image/jpeg"] = ".jpg";
    mimeTypes["image/png"] = ".png";
    mimeTypes["image/gif"] = ".gif";
    mimeTypes["image/bmp"] = ".bmp";
    mimeTypes["image/webp"] = ".webp";
    mimeTypes["image/svg+xml"] = ".svg";
    mimeTypes["image/x-icon"] = ".ico";
    mimeTypes["image/tiff"] = ".tif";

    // Application
    mimeTypes["application/pdf"] = ".pdf";
    mimeTypes["application/json"] = ".json";
    mimeTypes["application/xml"] = ".xml";
    mimeTypes["application/zip"] = ".zip";
    mimeTypes["application/x-7z-compressed"] = ".7z";
    mimeTypes["application/x-rar-compressed"] = ".rar";
    mimeTypes["application/gzip"] = ".gz";
    mimeTypes["application/x-tar"] = ".tar";
    mimeTypes["application/octet-stream"] = ".bin";

    // MS Office
    mimeTypes["application/msword"] = ".doc";
    mimeTypes["application/vnd.ms-excel"] = ".xls";
    mimeTypes["application/vnd.ms-powerpoint"] = ".ppt";

    // OOXML
    mimeTypes["application/vnd.openxmlformats-officedocument.wordprocessingml.document"] = ".docx";
    mimeTypes["application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"] = ".xlsx";
    mimeTypes["application/vnd.openxmlformats-officedocument.presentationml.presentation"] = ".pptx";

    // Audio
    mimeTypes["audio/mpeg"] = ".mp3";
    mimeTypes["audio/wav"] = ".wav";
    mimeTypes["audio/ogg"] = ".ogg";

    // Video
    mimeTypes["video/mp4"] = ".mp4";
    mimeTypes["video/x-msvideo"] = ".avi";
    mimeTypes["video/x-matroska"] = ".mkv";
    mimeTypes["video/webm"] = ".webm";
    mimeTypes["video/quicktime"] = ".mov";
}

serverConfig::~serverConfig(void) {}

void serverConfig::addLocation(std::string key, t_location loc)
{
	try
	{
		if (key.empty())
			throw errorHandler(INVALID_INSTRUCTION, "empty location");
		configUtils::ifDir(key);
	}
	catch(const std::exception& e)
	{
		throw errorHandler(std::string(e.what()));
	}
	locations[key] = loc;
}

void serverConfig::addRoute(std::string key, t_route route)
{
	if (key.empty())
		throw errorHandler(INVALID_INSTRUCTION, "empty route");
	if (key[0] != '/')
		throw errorHandler(INVALID_INSTRUCTION, key);
	
	routes[key] = route;
}

void serverConfig::setHost(t_host newHost) {host = newHost;}

void serverConfig::addErrorPages(unsigned short error, std::string page)
{
	std::stringstream ss;
	ss << error;
	std::map<unsigned short, std::string>::iterator res;
	res = errorPages.find(error);
	if (res == errorPages.end())
		throw errorHandler(WRONG_ERROR_CODE, ss.str());
	try
	{
		configUtils::ifFile(page);
	}
	catch(const std::exception& e)
	{
		throw errorHandler(WRONG_FILE, page);
	}
	errorPages[error] = page;
}

t_route serverConfig::getRoute(std::string route) const
{
	std::string err;
	std::map<std::string, t_route>::const_iterator res;

	err = "route " + route;
	res = routes.find(route);
	if (res == routes.end())
		throw errorHandler(NO_DATA, err);
	
	return res->second;
}

std::map<std::string, std::string>serverConfig::getEnv(void) const {return env;}

t_cgi serverConfig::getCgiConf(void) const {return cgiConf;}

t_host serverConfig::getHost(void) const {return host;}

std::map<std::string, t_location> serverConfig::getLocations(void) const {return locations;}

std::map<std::string, t_route> serverConfig::getRoutes(void) const {return routes;}

std::map<unsigned short, std::string> serverConfig::getErrorPages(void) const {return errorPages;}

std::string serverConfig::getErrorPage(unsigned short error) const {

	std::stringstream ss;
	std::string err;
	std::map<unsigned short, std::string>::const_iterator res;

	res = errorPages.find(error);
	if (res == errorPages.end())
	{
		err = "error code ";
		ss << error;
		err += ss.str();
		throw errorHandler(NO_DATA, err);
	}

	return res->second;
}

void serverConfig::checkConfig()
{
	std::map<std::string, t_location>::iterator lIt;
	std::map<std::string, t_route>::iterator rIt;
	
	for (lIt = locations.begin(); lIt != locations.end(); ++lIt)
	{
		if (lIt->second.empty())
		 throw errorHandler(MISSING_PROPERTY, " empty location");
	}
	for (rIt = routes.begin(); rIt != routes.end(); ++rIt)
	{
		if (rIt->second.empty())
			throw errorHandler(MISSING_PROPERTY, " empty route");
	}
	if(host.empty())
		throw errorHandler(MISSING_PROPERTY, " emty host config"); 
}

t_location serverConfig::getLocation(std::string location) 
{
	std::map<std::string, t_location>::iterator res = locations.find(location);
	if (res == locations.end())
		throw errorHandler(NO_DATA, location);
	return res->second;
}

bool serverConfig::checkMimeTypes(std::string const &type, std::string &ext)
{
	std::map<std::string, std::string>::iterator res = mimeTypes.find(type);
	if (res != mimeTypes.end())
	{
		ext = res->second;
		return true;
	}
	return false;
}

std::map<std::string, std::string> serverConfig::getMimeTypes() const {return mimeTypes;}
