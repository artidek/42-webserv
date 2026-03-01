/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serverConfig.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aobshatk <aobshatk@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/09 21:18:24 by aobshatk          #+#    #+#             */
/*   Updated: 2026/03/01 17:23:41 by aobshatk         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include "errorHandler.hpp"
#include <map>

#define MAX_REQUEST_BODY 1048576
#define GET "GET"
#define HEAD "HEAD"
#define POST "POST"
#define DELETE "DELETE"
#define OPTIONS "OPTIONS"
#define SRV "webserv"

typedef unsigned short timeout_t;

typedef struct s_host
{
	std::string addr;
	std::vector<std::string> ports;
	int maxReqBody;
	timeout_t hostTimeout;
	std::string hostname;
	bool empty();
} t_host;

typedef struct s_route
{
	std::string newRoot;
	std::string page;
	std::string response;
	std::vector<std::string> methods;
	std::string redirect;
	bool empty();
} t_route;

typedef struct s_cgi
{
	bool cgiAllowed;
	std::string root;
	std::string defaultCgi;
	std::vector<std::string> extensions;
	s_cgi &operator=(s_cgi const &copy);
	bool empty();
} t_cgi;

typedef struct s_location
{
	bool enableListing;
	bool enableUpload;
	std::vector<std::string> listExt;
	std::vector<std::string> uploadExt;
	bool empty();
}	t_location;

class serverConfig
{
	private:
		std::map<std::string, std::string> mimeTypes;
		std::map<std::string, t_location>locations;
		std::map<std::string, t_route>routes;
		std::map<unsigned short, std::string>errorPages;
		t_host host;
		static const std::map<std::string, std::string>env;
		t_cgi cgiConf;
		static std::map<std::string, std::string> makeEnv(void);
		void setMimeTypes();
	public:
		serverConfig(void);
		serverConfig(serverConfig const &copy);
		serverConfig &operator=(serverConfig const &copy);
		~serverConfig(void);
		void addLocation(std::string key, t_location loc);
		void addRoute(std::string key, t_route route);
		void setHost(t_host newHost);
		void addErrorPages(unsigned short error, std::string page);
		void setCgi(t_cgi cgiConf);
		void checkConfig();
		t_route getRoute(std::string route) const;
		t_location getLocation(std::string location);
		std::map <std::string, std::string> getEnv(void) const;
		t_cgi getCgiConf(void) const;
		t_host getHost(void) const;
		std::map<std::string, t_location> getLocations(void) const;
		std::map<std::string, t_route> getRoutes(void) const;
		const std::map<unsigned short, std::string> &getErrorPages(void) const;
		std::string getErrorPage(unsigned short error) const;
		bool checkMimeTypes(std::string const &type, std::string &ext);
		std::map<std::string, std::string> getMimeTypes() const;
};

#endif
