/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   configParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aobshatk <aobshatk@42warsaw.pl>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/10 12:37:36 by aobshatk          #+#    #+#             */
/*   Updated: 2025/11/10 14:15:29 by aobshatk         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include "serverConfig.hpp"
#include <deque>

class configParser
{
	private:
		serverConfig sc;
		std::vector<t_host> hosts;
		std::deque<std::string> tokens;
		std::string flattened;
		t_host host;
		t_location loc;
		t_route route;
		bool blockEnd;
		static std::vector<std::string> langBlocks;
		static std::vector<std::string> langBlockLocation;
		configParser(void);
		configParser(configParser const &copy);
		configParser &operator=(configParser const &copy);
		~configParser(void);
		void flatten(std::string confFile);
		void tokenize(void);
		void fillBlocks(void);
		void fillHostConf(void);
		void fillErrPg(void);
		void fillRoute(void);
		void fillLoc(void);
		void fillCgiConf(void);
	public:
		
};

#endif