/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   configParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aobshatk <aobshatk@42warsaw.pl>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/10 12:37:36 by aobshatk          #+#    #+#             */
/*   Updated: 2025/11/12 21:54:58 by aobshatk         ###   ########.fr       */
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
		static std::string blockProp;
		static bool blockEnd;
		static std::vector<serverConfig> hosts;
		static std::map<std::string, void(*)(std::stack<std::string>&)>blockNames;
		configParser(void);
		configParser(configParser const &copy);
		configParser &operator=(configParser const &copy);
		~configParser(void);
		static void flatten(std::ifstream const &file);
		static void tokenize(void);
		static void fillHostConf(std::stack<std::string> &blockTokens);
		static void fillErrPg(std::stack<std::string> &blockTokens);
		static void fillRoute(std::stack<std::string> &blockTokens);
		static void fillLoc(std::stack<std::string> &blockTokens);
		static void fillCgiConf(std::stack<std::string> &blockTokens);
		static void listToken(int &i);
		static void parseBlock(void);
		static void parseList(std::stack<std::string> &blockTokens);
		static void startBlock(void);
		static void checkBlock(std::stack<std::string>&blockTokens);
		static void initBlockNames(void);
	public:
		static void parseConfig(std::string confFile);
};

#endif