#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <unistd.h>
#include "Server.hpp"
#include "../../include/webserv.hpp"


Server::Server()
    :   _hostAddress(""),
        _listenPort(-1),
        _serverName(),
        _listenFd(-1),
        _started(false),
        _reusableAddress(false),
        _optionKeepAlive(false),
        _bytesSent(0),
        _bytesReceived(0)
{
    std::cout << "Server default Constructor called.\n";
}

Server::Server(const serverConfig& config)
    :   _hostAddress(config.endpoint.ip),
        _listenPort(config.endpoint.port),
        _serverName(config.serverNames),
        _listenFd(-1),
        _started(false),
        _reusableAddress(false),
        _optionKeepAlive(false),
        _bytesSent(0),
        _bytesReceived(0)
{
    std::cout << "Server Constructor called.\n";
}


Server::~Server()
{
    if (_listenFd != -1)
        close(_listenFd);
    std::cout << "Server Destructor called.\n";
}

void Server::setHostAddress(const std::string& address)
{
    _hostAddress = address;
}
const std::string& Server::getHostAddress() const
{
    return _hostAddress;
}

const std::vector<std::string>& Server::getServerNames() const
{
    return _serverName;
}

void Server::setListenPort(int port)
{
    _listenPort = port;
}

int Server::getListenPort() const
{
    return _listenPort;
}

bool Server::isStarted() const
{
    return _started;
}

int Server::getListenFd() const
{
    return _listenFd;
}

void Server::setReusableAddress(bool reusable)
{
    _reusableAddress = reusable;
}
bool Server::getReusableAddress() const
{
    return _reusableAddress;
}

void Server::setOptionKeepAlive(bool keepAlive)
{
    _optionKeepAlive = keepAlive;
}
bool Server::getOptionKeepAlive() const
{
    return _optionKeepAlive;
}

void Server::addClient(int socketFd)
{
    _clients.emplace(socketFd, std::make_unique<Client>(socketFd));

}

void Server::removeClient(int socketFd)
{
    _clients.erase(socketFd);
}

Client* Server::getClient(int socketFd)
{
    std::unordered_map<int, std::unique_ptr<Client>>::iterator it = _clients.find(socketFd);
    if (it != _clients.end()) {
        return it->second.get();
    }
    return NULL;
}

const std::unordered_map<int, std::unique_ptr<Client>>& Server::getClients() const
{
    return _clients;
}

void Server::addBytesSent(size_t bytes)
{
    _bytesSent += bytes;
}

size_t Server::getBytesSent() const
{
    return _bytesSent;
}
void Server::addBytesReceived(size_t bytes)
{
    _bytesReceived += bytes;
}

size_t Server::getBytesReceived() const
{
    return _bytesReceived;
} 

