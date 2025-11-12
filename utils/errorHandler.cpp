/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   errorHandler.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aobshatk <aobshatk@42warsaw.pl>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/09 20:51:37 by aobshatk          #+#    #+#             */
/*   Updated: 2025/11/11 20:19:45 by aobshatk         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/errorHandler.hpp"

std::string errorHandler::errHead[] = {"Error: Missing token", "Error: Invalid instruction", "Error: Invalid file", "Error: No data available", "Error: Wrong extension", "Error: Empty config not allowed", "Error: Missing property in", "Error: Wrong token"};

errorHandler::errorHandler(int errType, std::string err)
{
	msg = errHead[errType] + " => " + err;
}

errorHandler::errorHandler(int errTp) { msg = errHead[errTp];}

errorHandler::errorHandler(std::string err) : msg(err) {}

errorHandler::~errorHandler(void) throw() {}

const char *errorHandler::what(void) const throw() { return msg.c_str(); }
