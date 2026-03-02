/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   configParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aobshatk <aobshatk@42warsaw.pl>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/10 12:37:36 by aobshatk          #+#    #+#             */
/*   Updated: 2026/03/02 13:47:03 by aobshatk         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include "serverConfig.hpp"
#include <ctime>
#include <stack>

typedef std::pair<std::string, std::string> LookUpKey;

class configParser
{
	private:
		static std::stack<std::string> tokens;
		static std::string flattened;
		static std::string conf;
		static std::string blockProp;
		static bool blockEnd;
		static std::time_t timeout;
		static std::map<std::string, serverConfig> hosts;
		static std::map<std::string, void(*)(std::stack<std::string>&)>blockNames;
		static std::map<LookUpKey, std::string>virtualHosts;
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
		static void listToken(size_t &i, std::string &token);
		static void parseBlock(void);
		static void parseList(std::stack<std::string> &blockTokens);
		static void checkBlock(std::stack<std::string>&blockTokens);
		static void initBlockNames(void);
		static void extractBlockProp(size_t &i);
		static void addConfig();
		static void mergeConfigs(serverConfig const &conf, serverConfig &origConf);
	public:
		static void parseConfig(std::string confFile);
		static std::map<std::string, serverConfig> getConfigs(void);
		static bool checkBlocknames(std::string const &blockName);
		static std::time_t getTimeout();
};

#endif