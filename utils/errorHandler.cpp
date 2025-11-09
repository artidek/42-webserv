/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   errorHandler.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aobshatk <aobshatk@42warsaw.pl>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/09 20:51:37 by aobshatk          #+#    #+#             */
/*   Updated: 2025/11/09 21:17:28 by aobshatk         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/errorHandler.hpp"

std::vector<std::string> errorHandler::errHead = {"Error: Missing token", "Error: Invalid instruction"};

errorHandler::errorHandler(int errType, std::string err)
{
	std::stringstream ss(errHead[errType]);
	ss << " => " << err;
	ss >> msg;
}

const char *errorHandler::what(void) const throw() { return msg.c_str(); }
