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
		std::map<int, t_host> fdToHost;
		std::map<int, serverConfig> listenToHost;
		std::map<int, std::time_t>timeLog;
		std::vector<int> socketFds;
		std::vector<std::string> envp;
		std::set<std::pair<std::string, std::string> >uniqueAddr;
		std::vector<int>conSockFds;
		std::time_t reqTimeout;
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
		bool handleRequest(int const &fd);
		void handleResponse(int const &fd, requestHandler const &req);
		void closeConSock(int const &fd);
		void stopServer(int const &nfds, struct epoll_event *events);
		void armOut(int fd);
		void disarmOut(int fd);
		bool badResponse(int fd, std::string  const &err);
		void setAddresses();
		void addToTimeLog(int fd, std::time_t sec);
		void checkTimeout();
	public:
		server(std::map<std::string, serverConfig> const &conf, char **env);
		~server(void);
		void set(std::time_t timeout);
		void run();
		static void handle_signal(int sig);
};

#endif	
 