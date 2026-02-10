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
    int listenFd;
    std::vector<Server*> servers;
    Server* defaultServer;
};



#endif