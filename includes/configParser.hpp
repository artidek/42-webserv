/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   configParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aobshatk <aobshatk@42warsaw.pl>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/10 12:37:36 by aobshatk          #+#    #+#             */
/*   Updated: 2026/03/02 20:18:32 by aobshatk         ###   ########.fr       */
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
		std::stack<std::string> tokens;
		std::string flattened;
		std::string conf;
		std::string blockProp;
		bool blockEnd;
		std::time_t timeout;
		std::map<std::string, serverConfig> hosts;
		std::map<std::string, void(*)(std::stack<std::string>&)>blockNames;
		std::map<LookUpKey, std::string>virtualHosts;
		configParser(configParser const &copy);
		configParser &operator=(configParser const &copy);
		 void flatten(std::ifstream const &file);
		 void tokenize(void);
		 void fillHostConf(std::stack<std::string> &blockTokens);
		 void fillErrPg(std::stack<std::string> &blockTokens);
		 void fillRoute(std::stack<std::string> &blockTokens);
		 void fillLoc(std::stack<std::string> &blockTokens);
		 void fillCgiConf(std::stack<std::string> &blockTokens);
		 void listToken(size_t &i, std::string &token);
		 void parseBlock(void);
		 void parseList(std::stack<std::string> &blockTokens);
		 void checkBlock(std::stack<std::string>&blockTokens);
		 void initBlockNames(void);
		 void extractBlockProp(size_t &i);
		 void addConfig();
		 void mergeConfigs(serverConfig const &conf, serverConfig &origConf);
	public:
		configParser(void);
		~configParser(void);
		void parseConfig(std::string confFile);
		std::map<std::string, serverConfig> getConfigs(void);
		bool checkBlocknames(std::string const &blockName);
		std::time_t getTimeout();
		void clearResources();
};

#endif