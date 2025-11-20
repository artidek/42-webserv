#include "../includes/errorHandler.hpp"
#include "../includes/server.hpp"
#include "../includes/responseHandler.hpp"
#include <errno.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>

bool server::stop = false;
bool server::stopped = true;
int server::epollFd = -1;

server::server(std::vector<serverConfig> const &conf) : configs(conf)
{
}

server::~server(void)
{
}

void server::closeSfds(void)
{
	for (size_t i = 0; i < socketFds.size(); i++)
		close(socketFds[i]);
}

void server::freeInfos(std::vector<addrinfo *> &infos)
{
	for (size_t i = 0; i < infos.size(); i++)
		freeaddrinfo(infos[i]);
}

void server::setAddrInfo(std::vector<addrinfo *> &infos, t_host const &host)
{
	int				err;
	struct addrinfo	hints;
	struct addrinfo	*res;

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;
	hints.ai_addrlen = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	std::vector<std::string> p = host.ports;
	for (size_t i = 0; i < host.ports.size(); i++)
	{
		res = NULL;
		err = getaddrinfo(host.addr.c_str(), host.ports[i].c_str(), &hints,
				&res);
		if (err != 0)
			throw errorHandler(FAILED_MAP_ADDR, std::string(gai_strerror(err)));
		infos.push_back(res);
	}
}

void server::createSockets(serverConfig conf)
{
	int		sfd;
	t_host	host;

	std::vector<addrinfo *> infos;
	host = conf.getHost();
	try
	{
		setAddrInfo(infos, host);
		for (size_t i = 0; i < infos.size(); i++)
		{
			sfd = socket(infos[i]->ai_family, infos[i]->ai_socktype,
					infos[i]->ai_protocol);
			if (sfd == -1)
				throw errorHandler(SOCKET_FAILED, std::string(strerror(errno)));
			if (bind(sfd, infos[i]->ai_addr, infos[i]->ai_addrlen) == -1)
				throw errorHandler(BIND_FAILED, std::string(strerror(errno)));
			socketFds.push_back(sfd);
			listenToHost[sfd] = conf;
		}
	}
	catch (const std::exception &e)
	{
		freeInfos(infos);
		if (!socketFds.empty())
			closeSfds();
		throw errorHandler(std::string(e.what()));
	}
	freeInfos(infos);
}

void server::setNonBlocking(int &fd)
{
	int flags = fcntl(fd, F_GETFL, 0); //get all flags associated with file descriptor
	if (flags == -1) flags = 0; //if no flags available set to 0
	fcntl(fd, F_SETFL, flags | O_NONBLOCK); //set nonblock flag
}

void server::set()
{
	std::map<std::string, serverConfig>::iterator it;

	try
	{
		for (size_t i = 0; i < configs.size(); i++)
			createSockets(configs[i]);
		epollFd = epoll_create1(0);
		if (epollFd == -1)
			throw errorHandler(EPOLL_CREATE_FAIL, std::string(strerror(errno)));
		for (size_t i = 0; i < socketFds.size(); i++)
		{
			if (listen(socketFds[i], 4) == -1)
				throw errorHandler(PRT_MARK_FAILED, std::string(strerror(errno)));
			struct epoll_event ev;
			setNonBlocking(socketFds[i]);
			ev.events = EPOLLIN;
			ev.data.fd = socketFds[i];
			if (epoll_ctl(epollFd, EPOLL_CTL_ADD, socketFds[i], &ev) == -1)
				throw errorHandler(EPOLL_CREATE_FAIL, std::string(strerror(errno)));
		}
	}
	catch (const std::exception &e)
	{
		close(epollFd);
		closeSfds();
		throw errorHandler(std::string(e.what()));
	}
}

void server::readyEvents(int &nfds, struct epoll_event *events)
{
	nfds = epoll_wait(epollFd, events, MAX_EVENTS, 500); //wait for events and put them to events buffer
	if (nfds == - 1)
		throw errorHandler(EVENTS_FAILED, std::string(strerror(errno)));
}

bool server::listenSocket(int const &fd, serverConfig &conf)
{
	std::vector<int>::iterator res = std::find(socketFds.begin(), socketFds.end(), fd);
	if (res == socketFds.end())
		return false;
	std::map<int, serverConfig>::iterator resL = listenToHost.find(fd);
	if (resL != listenToHost.end())
		conf = resL->second;
	return true;
}

void server::handleClientData(int const &fd)
{
	std::map<int, serverConfig>::iterator res;
	res = fdToHost.find(fd);
	if (res != fdToHost.end())
	{
		try
		{
			requestHandler rH(res->second);
			rH.read(fd);
			//std::cout << rH.getRawData();
			rH.parse();
			responseHandler resp(res->second, rH.getReqData());
			resp.createResponce();
			// t_response response = resp.getResponceData();
			// std::cout << "respnose code " << response.respCode << std::endl;
			// std::map<std::string, std::string>::iterator it = response.headers.begin();
			// for(; it != response.headers.end(); ++it)
			// 	std::cout << it->first << " " << it->second << std::endl;
			// std::cout << "body: " << response.body;
			close(fd);
		}
		catch(const std::exception& e)
		{
			close(fd);
		}
	}
}

void server::proceedEvents(int const &nfds, struct epoll_event *events)
{
	int conn_socket;

	for (int n = 0; n < nfds; ++n)
	{
		int fd = events[n].data.fd;
		serverConfig conf;
		if (listenSocket(fd, conf))
		{
			while(true) //Accept all pending connections for the socket
			{
				struct sockaddr_in client_addr;
                socklen_t addrlen = sizeof(client_addr);
                conn_socket = accept(fd, (struct sockaddr*)&client_addr, &addrlen); //If connection exist will create a connections socket and return it's fd else return -1 in most of the cases signaling there is no pending connection
                if (conn_socket == -1) break;
				setNonBlocking(conn_socket); //sets connection socket nonblocking
				struct epoll_event ev_client;
            	ev_client.events = EPOLLIN | EPOLLET;  // edge-triggered read
            	ev_client.data.fd = conn_socket;
				if (epoll_ctl(epollFd, EPOLL_CTL_ADD, conn_socket, &ev_client) == -1) // put connection socket fd to epoll on error closes connection socket throws an error
					close(conn_socket);
				fdToHost[conn_socket] = conf;
			}
		}
		else
		{
			handleClientData(fd);
		}
	}
}

void server::run()
{
	int nfds;
	struct epoll_event events[MAX_EVENTS];
	while (!stop)
	{
		try
		{
			readyEvents(nfds, events);
			proceedEvents(nfds, events);
		}
		catch (const std::exception &e)
		{
			std::string err = "Server fatal error causing server stop: ";
			err += e.what();
			closeSfds();
			close(epollFd);
			throw errorHandler(err);
		}
	}
}
