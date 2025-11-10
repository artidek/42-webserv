/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   errorHandler.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aobshatk <aobshatk@42warsaw.pl>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/09 20:51:37 by aobshatk          #+#    #+#             */
/*   Updated: 2025/11/10 20:50:27 by aobshatk         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/errorHandler.hpp"

std::vector<std::string> errorHandler::errHead = {"Error: Missing token", "Error: Invalid instruction", "Error: Invalid file", "Error: No data available"};

errorHandler::errorHandler(int errType, std::string err)
{
	std::stringstream ss(errHead[errType]);
	ss << " => " << err;
	ss >> msg;
}

errorHandler::errorHandler(std::string err) : msg(err) {}

errorHandler::~errorHandler(void) {}

const char *errorHandler::what(void) const throw() { return msg.c_str(); }
