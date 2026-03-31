#ifndef WEBSERV_HPP
# define WEBSERV_HPP

# include <iostream>
# include <iomanip>
# include <cmath>
# include <stdio.h>
# include <cstdlib>
# include <unistd.h>
# include <cstring> 
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <netdb.h> 
# include <arpa/inet.h>
# include <vector>
# include <fcntl.h>
# include <poll.h>
# include <memory>
// # include "../src/client/Client.hpp"
// # include "../src/server/Server.hpp"
// # include "../src/manager/Manager.hpp"
# include "../src/configParser/parser.hpp"

class Server;

struct Listener
{
    serverEndpoint endpoint;
    int listenFd = -1;
    std::vector<Server*> servers;
    Server* defaultServer = nullptr;
};

std::vector<Listener> buildListeners(std::vector<std::unique_ptr<Server>>& servers);
bool    startListeners(std::vector<Listener>& listeners);
// void closeListenSockets(std::vector<Listener>& listeners)
// {
//     for (Listener& lis : listeners)
//     {
//         if (lis.listenFd != -1)
//         {
//             close(lis.listenFd);
//             lis.listenFd = -1;
//         }
//     }
// };
#endif