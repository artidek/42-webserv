/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serverConfig.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aobshatk <aobshatk@42warsaw.pl>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/09 21:18:24 by aobshatk          #+#    #+#             */
/*   Updated: 2025/11/09 21:45:54 by aobshatk         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include "errorHandler.hpp"

typedef struct s_listen_socket
{
	std::string addr;
	std::string port;
} t_listen_socket;

typedef struct s_route
{
	std::string newRoot;
	std::string page;
	std::string response;
} t_route;

typedef struct s_cgi
{
	bool allowed;
	std::vector<std::string> extensions;
	std::string root;
} t_cgi;

typedef struct s_location
{
	bool enableListing;
	bool enableUpload;
	std::vector<std::string> methods;
}	t_location;

#endif