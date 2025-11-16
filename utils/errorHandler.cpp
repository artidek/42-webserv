/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   errorHandler.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aobshatk <aobshatk@42warsaw.pl>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/09 20:51:37 by aobshatk          #+#    #+#             */
/*   Updated: 2025/11/16 21:44:03 by aobshatk         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/errorHandler.hpp"

std::string errorHandler::errHead[] = {"Error in config file: Missing token", 
	"Error in config file: Invalid instruction", 
	"Error in config file:: Invalid file ", 
	"Error in config file: No data available", 
	"Error in config file: Wrong extension", 
	"Error in config file: Empty config not allowed", 
	"Error in config file: Missing property in ", 
	"Error in config file: Wrong token",
	"Error in config file: Invalid http error code",
	"Error in config file: Invalid directory ",
	"Error setting server: failed to map addres ",
	"Error setting server: failed to create socket ",
	"Error setting server: failed to bind ",
	"Error setting server: failed to mark port ",
	"Error setting server: failed to create epoll " };

errorHandler::errorHandler(int errType, std::string err)
{
	msg = errHead[errType] + " => " + err;
}

errorHandler::errorHandler(int errTp) { msg = errHead[errTp];}

errorHandler::errorHandler(std::string err) : msg(err) {}

errorHandler::~errorHandler(void) throw() {}

const char *errorHandler::what(void) const throw() { return msg.c_str(); }
