#pragma once

#ifndef SERVER_H
#define SERVER_H

#include "../includes/serverConfig.hpp"
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>

#define MAX_EVENTS 10

class server
{
	private:
		static bool stop;
		std::map<std::string, serverConfig> configs;
		std::vector<int> socketFds;
		std::map<int, std::vector<struct epoll_event> > epollEvents;
		server(void);
		server(server const &copy);
		server &operator=(server const &copy);
		void createSockets(std::string const &addr);
		void setAddrInfo(std::vector<addrinfo*> &infos, t_host const &host);
		void freeInfos(std::vector<addrinfo*> &infos);
		void closeSfds(void);
		void closeEpollFds(void);
		void setWait(int &nfds, std::map<int, std::vector<struct epoll_event> >::iterator epoll);
	public:
		server(std::map<std::string, serverConfig> const &conf);
		~server(void);
		void set();
		void run();
};

#endif	