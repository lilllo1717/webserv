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
    :   _hostAddress("127.0.0.1"),
        _listenPort(8080),
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


// void Server::run()
// {
//     _poll_fds.clear();

//     struct sockaddr_in client_address;
// 	socklen_t	client_len;
// 	int new_socket_fd;

//     pollfd p;
//     p.fd = _listenFd;
//     p.events = POLLIN;
//     p.revents = 0;
//     _poll_fds.push_back(p);

//     while (_started)
//     {
//         // std::cerr << "we started.\n";
//         int ready_fds = poll(_poll_fds.data(), _poll_fds.size(), -1);
//         if (ready_fds < 0)
//         {
//             std::cerr << "poll failed.\n";
//             // close(_listenFd);
//             // _started = false;
//             break ;
//         }
//         for (size_t i = 0; i < _poll_fds.size(); i++)
//         {
//             if (_poll_fds[i].revents == 0)
//                 continue;

//             if (_poll_fds[i].fd == _listenFd && (_poll_fds[i].revents & POLLIN))
//             {
//                 while (true)
//                 {
//                     client_len = sizeof(client_address);
//                     new_socket_fd = accept(_listenFd, (struct sockaddr*)&client_address, &client_len);
//                     if (new_socket_fd < 0)
//                     {
//                         if (errno == EAGAIN || errno == EWOULDBLOCK)
//                             break;
//                         _started = false;
//                         break;
//                     }
//                     _poll_fds.push_back(pollfd{new_socket_fd, POLLIN, 0});
//                     addClient(new_socket_fd);
//                 }
//             }
//             else if (_poll_fds[i].fd != _listenFd && (_poll_fds[i].revents & POLLIN))
//             {
//                 char buffer[1024];
//                 ssize_t message_size = recv(_poll_fds[i].fd, buffer, sizeof(buffer), 0);
//                 int client_fd = _poll_fds[i].fd;
                
//                 if (message_size == 0)
//                 {
//                     // Client disconnected
//                     close(client_fd);
//                     removeClient(client_fd);
//                     _poll_fds.erase(_poll_fds.begin() + i);
//                     --i;
//                     continue;
//                 }
//                 else if (message_size < 0)
//                 {
//                     if (errno == EAGAIN || errno == EWOULDBLOCK)
//                     {
//                         // No data available right now, continue
//                         continue;
//                     }
//                     std::cerr << "recv error on fd " << client_fd << ": " << strerror(errno) << "\n";
//                     close(client_fd);
//                     removeClient(client_fd);
//                     _poll_fds.erase(_poll_fds.begin() + i);
//                     --i;
//                     continue;
//                 }

//                 Client* client = getClient(client_fd);
//                 if (!client)
//                     continue;
//                 client->appendToReceiveBuffer(std::string(buffer, message_size));
//                 client->appendToSendBuffer(std::string(buffer, message_size));
//                 _poll_fds[i].events |= POLLOUT;
//                 addBytesReceived(message_size);
//                 std::cout << "Received " << message_size << " bytes from client " << client_fd << "\n";

//             }

//             if (_poll_fds[i].fd != _listenFd && (_poll_fds[i].revents & POLLOUT))
//             { 
//                 int client_fd = _poll_fds[i].fd;
//                 Client* client = getClient(client_fd);
//                 if (!client)
//                     continue;
//                 const std::string& sendBuffer = client->getSendBuffer();
//                 if (sendBuffer.empty())
//                 {
//                     _poll_fds[i].events &= ~POLLOUT;
//                     continue;
//                 }
//                 ssize_t bytes_sent = send(client_fd, sendBuffer.c_str(), sendBuffer.size(), 0);
//                 if (bytes_sent < 0)
//                 {
//                     if (errno == EAGAIN || errno == EWOULDBLOCK)
//                     {
//                         continue;
//                     }
//                     std::cerr << "send error on fd " << client_fd << ": " << strerror(errno) << "\n";
//                     close(client_fd);
//                     removeClient(client_fd);
//                     _poll_fds.erase(_poll_fds.begin() + i);
//                     --i;
//                     continue;
//                 }
//                 client->appendToSendBuffer(sendBuffer.substr(bytes_sent));
//                 addBytesSent(bytes_sent);
//                 std::cout << "Sent " << bytes_sent << " bytes to client " << client_fd << "\n";
//             }
//             _poll_fds[i].revents = 0;
//         }
//     }
    
// }

// void Server::stop()
// {
//     _started = false;
//     if (_listenFd != -1)
//     {
//         close(_listenFd);
//     }

//     std::cout << "Server stopped.\n";
// }