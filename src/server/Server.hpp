

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <iomanip>
#include <map>
#include "../client/Client.hpp"
#include "../server/Server.hpp"

class Server
{
    private:
        std::string             _hostAddress;
        int                     _listenPort;
        int                     _listenFd;
        bool                    _started;
        bool                    _reusableAddress;
        bool                    _optionKeepAlive;
        std::map<int, Client>   _clients;
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
        const std::map<int, Client>& getClients() const;
        void addBytesSent(size_t bytes);
        size_t getBytesSent() const;

        void addBytesReceived(size_t bytes);
        size_t getBytesReceived() const;

        void start();
        void stop();

};
