/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serverConfig.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aobshatk <aobshatk@42warsaw.pl>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/09 21:18:24 by aobshatk          #+#    #+#             */
/*   Updated: 2025/11/10 21:31:36 by aobshatk         ###   ########.fr       */
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

typedef unsigned short timeout_t;

typedef struct s_host
{
	std::string hostName;
	std::string addr;
	std::string port;
	std::string defaulPage;
	int maxReqBody;
	timeout_t hostTimeout;
} t_host;

typedef struct s_route
{
	std::string newRoot;
	std::string page;
	std::string response;
} t_route;

typedef struct s_cgi
{
	bool cgiAllowed;
	std::vector<std::string> extensions;
	std::string root;
} t_cgi;

typedef struct s_location
{
	bool enableListing;
	bool enableUpload;
	std::vector<std::string> methods;
}	t_location;

class serverConfig
{
	private:
		std::map<std::string, t_location>locations;
		std::map<std::string, t_route>routes;
		std::map<unsigned short, std::string>errorPages;
		t_host host;
		static const std::map<std::string, std::string>env;
		static const std::vector<unsigned short>errorCodes;
		t_cgi cgiConf;
		serverConfig(serverConfig const &copy);
		static std::map<std::string, std::string> makeEnv(void);
	public:
		serverConfig(void);
		serverConfig &operator=(serverConfig const &copy);
		~serverConfig(void);
		void addLocation(std::string key, t_location loc);
		void addRoute(std::string key, t_route route);
		void setHost(t_host newHost);
		void addErrorPages(unsigned short error, std::string page);
		void setCgi(t_cgi cgiConf);
		t_route getRoute(std::string route) const;
		t_location getLocation(std::string location) const;
		std::map <std::string, std::string> getEnv(void) const;
		t_cgi getCgiConf(void) const;
		t_host getHost(void) const;
		std::map<std::string, t_location> getLocations(void) const;
		std::map<std::string, t_route> getRoutes(void) const;
		std::map<unsigned short, std::string> getErrorPages(void) const;
		std::string getErrorPage(unsigned short error) const;
};

#endif