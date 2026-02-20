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
#include <set>


#define MAX_EVENTS 10

class server
{
	private:
		static bool stop;
		static bool stopped;
		static int epollFd;
		struct epoll_event event;
		std::map<int, requestHandler> pendingRequests;
		std::map<int, responseHandler> pendingResponses;
		std::map<std::string, serverConfig> configs;
		std::map<int, serverConfig> fdToHost;
		std::map<int, serverConfig> listenToHost;
		std::vector<int> socketFds;
		std::vector<std::string> envp;
		std::set<std::pair<std::string, std::string> >uniqueAddr;
		std::vector<int>conSockFds;
		server(void);
		server(server const &copy);
		server &operator=(server const &copy);
		void createSockets();
		void setAddrInfo(std::vector<addrinfo*> &infos);
		void freeInfos(std::vector<addrinfo*> &infos);
		void closeSfds(void);
		void setNonBlocking(int &fd);
		void readyEvents(int &nfds, struct epoll_event *events);
		void proceedEvents(int const &nfds, struct epoll_event *events);
		bool listenSocket(int const &fd, serverConfig &conf);
		void handleClientData(int const &fd);
		bool isPendingReq(int const &fd);
		void handleRequest(int const &fd, requestHandler &rH);
		void handleResponse(int const &fd, requestHandler const &req);
		void closeConSock(int const &fd);
		void stopServer();
		void armOut(int fd);
		void disarmOut(int fd);
		bool badResponse(int fd, std::string  const &err);
		void setAddresses();
	public:
		server(std::map<std::string, serverConfig> const &conf, char **env);
		~server(void);
		void set();
		void run();
		static void handle_signal(int sig);
};

#endif	
 