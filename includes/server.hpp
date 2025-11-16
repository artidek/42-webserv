#pragma once

#ifndef SERVER_H
#define SERVER_H

#include "../includes/serverConfig.hpp"
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

class server
{
	private:
		std::map<std::string, serverConfig> configs;
		std::vector<int> socketFds;
		server(void);
		server(server const &copy);
		server &operator=(server const &copy);
		void createSockets(std::string const &addr);
		void setAddrInfo(std::vector<addrinfo*> &infos, t_host const &host);
		void freeInfos(std::vector<addrinfo*> &infos);
		void closeSfds(void);
	public:
		server(std::map<std::string, serverConfig> const &conf);
		~server(void);
		void set();
};

#endif	