#include "webserv.hpp"
# include "../src/client/Client.hpp"
# include "../src/server/Server.hpp"
# include "../src/manager/Manager.hpp"

std::vector<Listener> buildListeners(std::vector<std::unique_ptr<Server>>& servers)
{
	std::vector<Listener> listeners;

	for (size_t i = 0; i < servers.size(); i++)
	{
		std::string ip = servers[i]->getHostAddress();
		int port = servers[i]->getListenPort();
		bool found = false;
		for (size_t j = 0; j < listeners.size(); j++)
		{
			if (ip == listeners[j].endpoint.ip && port == listeners[j].endpoint.port)
			{
				listeners[j].servers.push_back(servers[i].get());
				found = true;
				break;
			}

		}
		if (!found)
		{
			Listener newListener;
			newListener.endpoint.ip = ip;
			newListener.endpoint.port = port;
			newListener.servers.push_back(servers[i].get());
			newListener.defaultServer = servers[i].get();
			listeners.push_back(newListener);
		}
	}
	return listeners;
}

void closeListenSockets(std::vector<Listener>& listeners)
{
    for (Listener& lis : listeners)
    {
        if (lis.listenFd != -1)
        {
            close(lis.listenFd);
            lis.listenFd = -1;
        }
    }
}


bool    startListeners(std::vector<Listener>& listeners)
{
    for (Listener& lis : listeners)
    {
        lis.listenFd = socket(AF_INET, SOCK_STREAM, 0);
        if (lis.listenFd < 0)
        {
            std::cerr << "Socket creation failed.\n";
            closeListenSockets(listeners);
            return false;
        }
        if (fcntl(lis.listenFd, F_SETFL, O_NONBLOCK) == -1)
        {
            std::cerr << "fcntl failed.\n";
            closeListenSockets(listeners);
            return false;
        }
        /*
            Mark this file descriptor so it is automatically closed on exec()
            fdtable[listenFd].flags |= FD_CLOEXEC
        */
        if (fcntl(lis.listenFd, F_SETFD, FD_CLOEXEC) == -1)
        {
            std::cerr << "fcntl failed.\n";
            closeListenSockets(listeners);
            return false;
        }
        int opt = 1;
        if (setsockopt(lis.listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        {
            std::cerr << "setsockopt failed.\n";
            closeListenSockets(listeners);
            return false;
        }
        struct sockaddr_in server_address;
        std::memset(&server_address, 0, sizeof(server_address));
        server_address.sin_family = AF_INET;
        server_address.sin_addr.s_addr = inet_addr(lis.endpoint.ip.c_str());
        server_address.sin_port = htons(lis.endpoint.port);

        if (bind(lis.listenFd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0)
        {
            std::cerr << "bind failed.\n";
            closeListenSockets(listeners);
            return false;
        }

        if (listen(lis.listenFd, 10) < 0)
        {
            std::cerr << "listen to socket failed.\n";
            closeListenSockets(listeners);
            return false;
        }

        std::cout << "Listening on " << lis.endpoint.ip << ":" << lis.endpoint.port
                    << " fd=" << lis.listenFd << "\n";
    }
    return true;
}
