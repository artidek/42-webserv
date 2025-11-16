#include "../includes/server.hpp"
#include <iostream>
#include "../includes/errorHandler.hpp"
#include <string.h>
#include <unistd.h>
#include <errno.h>

server::server(std::map<std::string, serverConfig> const &conf) : configs(conf) {}

server::~server(void) {}

void server::closeSfds(void)
{
	for (size_t i = 0; i < socketFds.size(); i++)
		close(socketFds[i]);
}

void server::freeInfos(std::vector<addrinfo*> &infos)
{
	for (size_t i = 0; i < infos.size(); i++)
		freeaddrinfo(infos[i]);
}

void server::setAddrInfo(std::vector<addrinfo*> &infos, t_host const &host)
{
	int err;

	struct addrinfo hints;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;
	hints.ai_addrlen = 0;
	hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
	std::vector<std::string>p = host.ports;
	for (size_t i = 0; i < host.ports.size(); i++)
	{
		struct addrinfo *res = NULL;
		err = getaddrinfo(host.addr.c_str(), host.ports[i].c_str(), &hints, &res);
		if (err != 0)
			throw errorHandler(FAILED_MAP_ADDR, std::string(gai_strerror(err)));
		infos.push_back(res);
	}
}

void server::createSockets(std::string const &addr)
{
	int sfd;
	std::map<std::string, serverConfig>::iterator find;
	t_host host;
	std::vector<addrinfo*>infos;

	find = configs.find(addr);
	host = find->second.getHost();
	try
	{
		setAddrInfo(infos, host);
		for (size_t i = 0; i < infos.size(); i++)
		{
			sfd = socket(infos[i]->ai_family, infos[i]->ai_socktype, infos[i]->ai_protocol);
			if (sfd == -1)
				throw errorHandler(SOCKET_FAILED, std::string(strerror(errno)));
			if (bind(sfd, infos[i]->ai_addr, infos[i]->ai_addrlen) == -1)
				throw errorHandler(BIND_FAILED, std::string(strerror(errno)));
			socketFds.push_back(sfd);
		}
		
	}
	catch(const std::exception& e)
	{
		freeInfos(infos);
		if (!socketFds.empty())
			closeSfds();
		throw errorHandler(std::string(e.what()));
	}
	freeInfos(infos);
}

void server::set ()
{
	std::map<std::string, serverConfig>::iterator it;

	try
	{
		for (it = configs.begin(); it != configs.end(); ++it)
			createSockets(it->first);
		for (size_t i = 0; i < socketFds.size(); i++)
		{
			if (listen(socketFds[i], 4) == -1)
				throw errorHandler(PRT_MARK_FAILED, std::string(strerror(errno)));
		}
	}
	catch(const std::exception& e)
	{
		closeSfds();
		throw errorHandler(std::string(e.what()));
	}
	
}