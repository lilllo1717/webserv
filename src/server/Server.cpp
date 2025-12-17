#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "Server.hpp"
#include "../../include/webserv.hpp"


Server::Server()
    :   _hostAddress(""),
        _listenPort(0),
        _listenFd(-1),
        _started(false),
        _reusableAddress(false),
        _optionKeepAlive(false),
        _bytesSent(0),
        _bytesReceived(0)
{
    std::cout << "Server default Constructor called.\n";
}

Server& Server::operator=(const Server& other)
{
    if (this != &other) {
        _hostAddress = other._hostAddress;
        _listenPort = other._listenPort;
        _started = other._started;
        _listenFd = other._listenFd;
        _reusableAddress = other._reusableAddress;
        _optionKeepAlive = other._optionKeepAlive;
        _clients = other._clients;
        _bytesSent = other._bytesSent;
        _bytesReceived = other._bytesReceived;
    }
    return *this;
}
Server::~Server()
{
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
    _clients.emplace(socketFd, Client(socketFd));
}

void Server::removeClient(int socketFd)
{
    _clients.erase(socketFd);
}

Client* Server::getClient(int socketFd)
{
    std::map<int, Client>::iterator it = _clients.find(socketFd);
    if (it != _clients.end()) {
        return &(it->second);
    }
    return NULL;
}

const std::map<int, Client>& Server::getClients() const
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

void Server::start()
{
    
    _listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_listenFd < 0)
    {
        std::cerr << "Socket creation failed.\n";
        _started = false;
        return;
    }
    if (fcntl(_listenFd, F_SETFL, O_NONBLOCK) == -1)
    {
        std::cerr << "fcntl failed.\n";
        _started = false;
        return;
    }
    /*
        Mark this file descriptor so it is automatically closed on exec()
        fdtable[listenFd].flags |= FD_CLOEXEC
    */
    if (fcntl(_listenFd, F_SETFD, FD_CLOEXEC) == -1)
    {
        std::cerr << "fcntl failed.\n";
        _started = false;
        return;
    }
    int opt = 1;
    if (setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		std::cerr << "setsockopt failed.\n";
        close(_listenFd);
        _started = false;
		return ;
	}
    struct sockaddr_in server_address;
    std::memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(_hostAddress.c_str());
    server_address.sin_port = htons(_listenPort);

    if (bind(_listenFd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0)
    {
        std::cerr << "bind failed.\n";
        close(_listenFd);
        _started = false;
        return;
    }

    if (listen(_listenFd, 10) < 0)
    {
        std::cerr << "listen to socket failed.\n";
        close(_listenFd);
        _started = false;
        return;
    }

    _bytesReceived = 0;
    _bytesSent = 0;

    _started = true;
    std::cout << "Server started on " << _hostAddress << ":" << _listenPort << "\n";

}

void Server::stop()
{
    _started = false;
    std::cout << "Server stopped.\n";
}