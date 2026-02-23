#include "../includes/errorHandler.hpp"
#include "../includes/server.hpp"
#include "../includes/configUtils.hpp"
#include <errno.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>

bool server::stop = false;
bool server::stopped = true;
int server::epollFd = -1;

server::server(std::map<std::string, serverConfig> const &conf, char **env) : configs(conf)
{
	for (int i = 0; env[i]; i++)
		envp.push_back(std::string(env[i]));
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

void server::setAddrInfo(std::vector<addrinfo *> &infos)
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
	std::set<std::pair<std::string, std::string> >::iterator it = uniqueAddr.begin();
	for (; it != uniqueAddr.end(); ++it)
	{
		res = NULL;
		err = getaddrinfo(it->first.c_str(), it->second.c_str(), &hints,
				&res);
		if (err != 0)
			throw errorHandler(FAILED_MAP_ADDR, std::string(gai_strerror(err)));
		infos.push_back(res);
	}
}

void server::createSockets()
{
	int		sfd;

	std::vector<addrinfo *> infos;
	int yes = 1;
	try
	{
		setAddrInfo(infos);
		for (size_t i = 0; i < infos.size(); i++)
		{
			sfd = socket(infos[i]->ai_family, infos[i]->ai_socktype,
					infos[i]->ai_protocol);
			setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
			if (sfd == -1)
				throw errorHandler(SOCKET_FAILED, std::string(strerror(errno)));
			if (bind(sfd, infos[i]->ai_addr, infos[i]->ai_addrlen) == -1)
				throw errorHandler(BIND_FAILED, std::string(strerror(errno)));
			socketFds.push_back(sfd);
			//listenToHost[sfd] = conf;
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
	setAddresses();
	try
	{
		//for (size_t i = 0; i < configs.size(); i++)
		createSockets();
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
	nfds = epoll_wait(epollFd, events, MAX_EVENTS, -1); //wait for events and put them to events buffer
	if (nfds == - 1)
	{
		if (!(errno == EINTR && stop))
			throw errorHandler(EVENTS_FAILED, std::string(strerror(errno)));
	}
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

bool server::isPendingReq(int const &fd)
{
	std::map<int, requestHandler>::iterator res = pendingRequests.find(fd);
	if (res == pendingRequests.end())
		return false;
	return true;
}

void server::handleResponse(int const &fd, requestHandler const &req)
{
	responseHandler resp(req);
	try
	{
		resp.createResponce(envp);
		resp.sendResponse(fd);
		if (resp.responseComplete())
			closeConSock(fd);
		else
		{
			pendingResponses[fd] = resp;
			armOut(fd);
		}
	}
	catch(const std::exception& e)
	{
		std::string err(e.what());
		throw errorHandler(err);
	}
}

void server::handleClientData(int const &fd)
{
	requestHandler  req;
	if (isPendingReq(fd))
		req = pendingRequests[fd];
	try
	{
		if (event.events & EPOLLIN)
		{
			req.read(fd);
			if (req.requestComplete())
			{
				timeLog.erase(fd);
				pendingRequests.erase(fd);
				req.parse();
				handleResponse(fd, req);
			}
			if (!isPendingReq(fd))
			{
				pendingRequests[fd] = req;
				addToTimeLog(fd, configUtils::getTime());
			}
			checkTimeout(fd, req.getConfig().getHost().hostTimeout);
		}
		if (event.events & EPOLLOUT)
		{
			std::map<int, responseHandler>::iterator resResp = pendingResponses.find(fd);
			responseHandler &resp = resResp->second;
			resp.sendToClient(fd);
			if (resp.responseComplete())
			{
					disarmOut(fd);
					closeConSock(fd);
					pendingResponses.erase(fd);
			}
		}
		}
		catch(const std::exception& e)
		{
			std::string err(e.what());
			if (err != "Send failed" && err != "Peer closed" && err != "Client closed connection")
			{
				responseHandler badResp;
				int rc = badResp.findRespCode(err);
				if (rc > 0)
					badResp.sendBad(rc, fd);
				else
					badResp.sendBad(400, fd);
				if (isPendingReq(fd))
				{
					timeLog.erase(fd);
					pendingRequests.erase(fd);
				}
				closeConSock(fd);
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
            	ev_client.events = EPOLLIN | EPOLLET;  
            	ev_client.data.fd = conn_socket;
				if (epoll_ctl(epollFd, EPOLL_CTL_ADD, conn_socket, &ev_client) == -1) // put connection socket fd to epoll on error closes connection socket throws an error
					close(conn_socket);
				//fdToHost[conn_socket] = conf;
			}
		}
		else
		{
			event = events[n];
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
			stopServer();
			throw errorHandler(err);
		}
	}
	stopServer();
}

void server::closeConSock(int const &fd)
{
	if (epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL) == -1)
		std::cout << "epoll_ctl_del failed\n";
	close(fd);
	fdToHost.erase(fd);
}

void server::handle_signal(int sig)
{
	if (sig)
		stop = true;
}

void server::stopServer()
{
	closeSfds();
	close(epollFd);
}

void server::armOut(int fd)
{
	event.events = EPOLLIN | EPOLLET | EPOLLOUT;
	epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &event);
}

void server::disarmOut(int fd)
{
	event.events = EPOLLIN | EPOLLET;
	epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &event);
}

void server::setAddresses()
{
	std::map<std::string, serverConfig>::iterator it = configs.begin();
	for (; it != configs.end(); ++it)
	{
		t_host h =  it->second.getHost();
		std::string ip = h.addr;
		std::vector<std::string> ports = h.ports;
		for (size_t j = 0; j < ports.size(); j++)
			uniqueAddr.insert(std::make_pair(ip, ports[j]));
	}
}

void server::addToTimeLog(int fd, std::time_t sec)
{
	std::map<int, std::time_t>::iterator res = timeLog.find(fd);
	if (res == timeLog.end())
		timeLog[fd] = sec;
}

void server::checkTimeout(int fd, int timeOut)
{
	std::time_t now = std::time(NULL);
	std::cout << timeOut << std::endl;
	if ( now - timeLog[fd] > timeOut)
	{
		timeLog.erase(fd);
		throw errorHandler("Request Timeout");
	}
}