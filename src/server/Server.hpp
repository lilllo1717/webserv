#ifndef SERVER_HPP
# define SERVER_HPP


#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <vector>
#include <poll.h>
#include <iomanip>
#include <map>
#include <unordered_map>
#include "../client/Client.hpp"

class Server
{
    private:
        std::string             _hostAddress;
        int                     _listenPort;
        int                     _listenFd;
        bool                    _started;
        bool                    _reusableAddress;
        bool                    _optionKeepAlive;
        std::unordered_map<int, Client>   _clients;
        std::vector<pollfd>     _poll_fds;
        size_t                  _bytesSent;
        size_t                  _bytesReceived;



    protected:
 

    public:
        Server();
        Server& operator=(const Server& other);
        virtual ~Server();

        void setHostAddress(const std::string& address);
        const std::string& getHostAddress() const;

        void setListenPort(int port);
        int getListenPort() const;

        int getListenFd() const;
        bool isStarted() const;

        void setReusableAddress(bool reusable);
        bool getReusableAddress() const;

        void setOptionKeepAlive(bool keepAlive);
        bool getOptionKeepAlive() const;

        void addClient(int socketFd);
        void removeClient(int socketFd);
        Client* getClient(int socketFd);
        const std::unordered_map<int, Client>& getClients() const;
        void addBytesSent(size_t bytes);
        size_t getBytesSent() const;

        void addBytesReceived(size_t bytes);
        size_t getBytesReceived() const;

        void start();
        void run();
        void stop();

};

#endif