#ifndef MANAGER_HPP
# define MANAGER_HPP

# include <stdio.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <iostream>
# include <iomanip>
# include <map>
# include <unordered_map>
# include <memory>
# include "../client/Client.hpp"
# include "../server/Server.hpp"


class Manager
{
    private:
        std::vector<std::unique_ptr<Server>> _servers;
        std::unordered_map<int, std::unique_ptr<Client>>   _clients; // key: client socket fd, value: client pointer
        std::unordered_map<int, Server*>   _clientFdToServer; // key: client socket fd, value: server pointer
        std::unordered_map<int, std::vector<Server*>> _listenFdToServers;

        std::vector<pollfd>     _poll_fds;

    public:
        Manager();
        virtual ~Manager();

        Manager& operator=(const Manager&) = delete;
        Manager(const Manager&) = delete;
        
        void addServer(std::unique_ptr<Server> server);
        std::vector<std::unique_ptr<Server>>& getServers();
        // std::unordered_map<int, std::unique_ptr<Client>>& getClients();
        // std::unordered_map<int, Server*>& getClientFdToServer();
        // std::unordered_map<int, std::vector<Server*>>& getListenFdToServers();
        void run();
};


#endif