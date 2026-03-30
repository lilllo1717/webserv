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
# include "../configParser/parser.hpp"
# include "../http/Router.hpp"
# include <arpa/inet.h>


struct Listener;
class Client;
class Router;

class Manager
{
    private:
        ssize_t  _recvBufferSize;
        ssize_t  _maxConnections;

        std::vector<std::unique_ptr<Server>> _servers;
        std::unordered_map<int, std::unique_ptr<Client>>   _clients; // key: client socket fd, value: client pointer
        std::unordered_map<int, Server*>   _clientFdToServer; // key: client socket fd, value: server pointer
        // std::unordered_map<int, std::vector<Server*>> _listenFdToServers;
        
        std::vector<pollfd>     _poll_fds;
        std::vector<Listener> _listeners;
        Router                 _router;

        std::unordered_map<int, size_t> _listenFdtoListenerIndex; // key: listen socket fd, value: index in _listeners vector
        std::unordered_map<int, size_t> _clientFdToListenerIndex;

        std::map<int, std::string>  _clientFdToRemoteAddress;

    public:
        Manager();
        virtual ~Manager();

        Manager& operator=(const Manager&) = delete;
        Manager(const Manager&) = delete;
        
        void addServer(std::unique_ptr<Server> server);
        // void addClient(int socketFd, Server* server);
        void addClient(int socketFd);
        void removeClient(int socketFd);
        Client* getClient(int socketFd);
        Server* getServerByClientFd(int socketFd);
        void setListeners();
        std::vector<Listener>& getListeners();
        std::vector<std::unique_ptr<Server>>& getServers();
        void buildListenersFromServers();
        int startListenersServers();
        // std::unordered_map<int, std::unique_ptr<Client>>& getClients();
        // std::unordered_map<int, Server*>& getClientFdToServer();
        // std::unordered_map<int, std::vector<Server*>>& getListenFdToServers();
        int run();
        void initializePollFds();
        void acceptNewConnection(int listenerFd, size_t listenerIndex);
        void receiveDataFromClient(size_t& i);
        void processClientRequest(size_t& i, char* temp_buffer, ssize_t message_size);
        void responseToClient(size_t& i);

        void closeListenSockets(std::vector<Listener>& listeners);
};


#endif