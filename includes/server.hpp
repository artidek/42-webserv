#pragma once

#ifndef SERVER_H
#define SERVER_H

#include "serverConfig.hpp"
#include "requestHandler.hpp"
#include "responseHandler.hpp"
#include "configUtils.hpp"
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>

#define MAX_EVENTS 10

class server
{
	private:
		static bool stop;
		static bool stopped;
		static int epollFd;
		std::map<int, requestHandler> pendingRequests;
		std::vector<serverConfig> configs;
		std::map<int, serverConfig> fdToHost;
		std::map<int, serverConfig> listenToHost;
		std::vector<int> socketFds;
		server(void);
		server(server const &copy);
		server &operator=(server const &copy);
		void createSockets(serverConfig conf);
		void setAddrInfo(std::vector<addrinfo*> &infos, t_host const &host);
		void freeInfos(std::vector<addrinfo*> &infos);
		void closeSfds(void);
		void setNonBlocking(int &fd);
		void readyEvents(int &nfds, struct epoll_event *events);
		void proceedEvents(int const &nfds, struct epoll_event *events);
		bool listenSocket(int const &fd, serverConfig &conf);
		void handleClientData(int const &fd);
		bool isPendingReq(int const &fd, requestHandler &req);
		void handleRequest(int const &fd, serverConfig const &conf, requestHandler &rH);
		void handleResponse(int const &fd, serverConfig const &conf, requestHandler const &req);
	public:
		server(std::vector<serverConfig> const &conf);
		~server(void);
		void set();
		void run();
};

#endif	