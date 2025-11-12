/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   configParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aobshatk <aobshatk@42warsaw.pl>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/10 12:37:36 by aobshatk          #+#    #+#             */
/*   Updated: 2025/11/12 11:33:02 by aobshatk         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include "serverConfig.hpp"
#include <stack>

class configParser
{
	private:
		static std::stack<std::string> tokens;
		static std::string flattened;
		static bool blockEnd;
		static std::vector<std::string> langBlocks;
		static std::vector<std::string> langBlockLocation;
		configParser(void);
		configParser(configParser const &copy);
		configParser &operator=(configParser const &copy);
		~configParser(void);
		static void flatten(std::ifstream const &file);
		static void tokenize(void);
		static void fillHostConf(void);
		static void fillErrPg(void);
		static void fillRoute(void);
		static void fillLoc(void);
		static void fillCgiConf(void);
		static void listToken(int &i);
	public:
		static std::vector<serverConfig> parseConfig(std::string confFile);
};

#endif