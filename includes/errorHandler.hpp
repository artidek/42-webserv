/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   errorHandler.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aobshatk <aobshatk@42warsaw.pl>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/09 20:51:09 by aobshatk          #+#    #+#             */
/*   Updated: 2025/11/14 20:11:43 by aobshatk         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <exception>
#include <vector>
#include <string>
#include <sstream>

enum {

	MISSING_TOKEN,
	INVALID_INSTRUCTION,
	WRONG_FILE,
	NO_DATA,
	WRONG_EXT,
	CONFIG_EMPTY,
	MISSING_PROPERTY,
	WRONG_TOKEN,
	WRONG_ERROR_CODE,
	INVALID_DIR
};

class errorHandler : public std::exception
{
	private:
		static std::string errHead[];
		std::string msg;
	public:
		errorHandler(int errTp);
		errorHandler(int errType, std::string err);
		errorHandler(std::string err);
		~errorHandler() throw();
		const char *what() const throw();
};


#endif