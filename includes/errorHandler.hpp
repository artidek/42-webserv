/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   errorHandler.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aobshatk <aobshatk@42warsaw.pl>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/09 20:51:09 by aobshatk          #+#    #+#             */
/*   Updated: 2025/11/10 20:49:52 by aobshatk         ###   ########.fr       */
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
	NO_DATA
};

class errorHandler : public std::exception
{
	private:
		static std::vector<std::string> errHead;
		std::string msg;
	public:
		errorHandler(int errType, std::string err);
		errorHandler(std::string err);
		virtual ~errorHandler();
		const char *what() const throw();
};


#endif