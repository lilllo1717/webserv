
#include "Manager.hpp"
#include "../../include/webserv.hpp"


Manager::Manager()
    :
        _recvBufferSize(30000),
        _maxConnections(128),
        _servers(),
        _clients(),
        _clientFdToServer(),
        // _listenFdToServers(),
        _poll_fds(),
        _listeners(),
        _listenFdtoListenerIndex(),
        _clientFdToRemoteAddress()
    
{
    std::cout << "Manager Constructor called.\n";
}

Manager::~Manager()
{
    std::cout << "Manager Destructor called.\n";
}

void Manager::addServer(std::unique_ptr<Server> server)
{
    // int listenFd = server->getListenFd();
    // _listenFdToServers[listenFd].push_back(server.get());
    _servers.push_back(std::move(server));
}

std::vector<std::unique_ptr<Server>>& Manager::getServers()
{
    return _servers;
}

void Manager::addClient(int socketFd)
{
    _clients[socketFd] = std::make_unique<Client>(socketFd);
    // _clientFdToServer[socketFd] = server;
}

void Manager::removeClient(int socketFd)
{
    _clients.erase(socketFd);
    _clientFdToServer.erase(socketFd);
}

Client* Manager::getClient(int socketFd)
{
    auto it  = _clients.find(socketFd);
    if (it == _clients.end())
        return nullptr;
    return it->second.get();
}

void Manager::setListeners()
{

    _listenFdtoListenerIndex.clear();
    for (size_t i = 0; i < _listeners.size(); i++)
    {
        _listenFdtoListenerIndex[_listeners[i].listenFd] = i;
    }
    
}

void Manager::buildListenersFromServers()
{
    _listeners = buildListeners(_servers);
}

int Manager::startListenersServers()
{
    if (startListeners(_listeners) == false)
        return -1;
    setListeners();
    return 1;
}

std::vector<Listener>& Manager::getListeners()
{
    return _listeners;
}

void Manager::acceptNewConnection(int listenerFd, size_t listenerIndex)
{
	socklen_t	client_len;

    while (true)
    {
        struct sockaddr_in client_address;
        client_len = sizeof(client_address);
        int new_socket_fd = accept(listenerFd, (struct sockaddr*)&client_address, &client_len);
        
        if (new_socket_fd < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            // _started = false;
            break;
        }
        int flags = fcntl(new_socket_fd, F_GETFL, 0);
        flags = flags | O_NONBLOCK;
        fcntl(new_socket_fd, F_SETFL, flags);

        int fdflags = fcntl(new_socket_fd, F_GETFD, 0);
        fdflags = fdflags | FD_CLOEXEC;
        fcntl(new_socket_fd, F_SETFD, fdflags);
        
        _clientFdToRemoteAddress[new_socket_fd] = inet_ntoa(client_address.sin_addr);
        _poll_fds.push_back(pollfd{new_socket_fd, POLLIN, 0});
        addClient(new_socket_fd);
        _clientFdToListenerIndex[new_socket_fd] = listenerIndex;
    }
}

void Manager::responseToClient(size_t& i)
{

    int client_fd = _poll_fds[i].fd;
    Client* client = getClient(client_fd);
    if (!client)
        return ;
    const std::string& sendBuffer = client->getSendBuffer();
    if (sendBuffer.empty())
    {
        close(client_fd);
        removeClient(client_fd);
        _poll_fds.erase(_poll_fds.begin() + i);
        // _poll_fds[i].events &= ~POLLOUT;
        --i;
        return ;
    }
    ssize_t bytes_sent = send(client_fd, sendBuffer.c_str(), sendBuffer.size(), 0);
    if (bytes_sent < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return ;
        }
        std::cerr << "send error on fd " << client_fd << ": " << strerror(errno) << "\n";
        close(client_fd);
        removeClient(client_fd);
        _poll_fds.erase(_poll_fds.begin() + i);
        --i;
        return ;
    }

    client->clearBuffer(bytes_sent);
    if (client->getSendBuffer().empty())
    {
        close(client_fd);
        removeClient(client_fd);
        _poll_fds.erase(_poll_fds.begin() + i);
        --i;
        return;
    }
    
}

// void Manager::processClientRequest(size_t& i, std::string temp_buffer, ssize_t message_size)


void Manager::processClientRequest(size_t& i, char* temp_buffer, ssize_t message_size)
{
    int client_fd = _poll_fds[i].fd;
    Client* client = getClient(client_fd);
    if (!client)
        return;
    HttpRequest& request = client->getHttpRequest();
    // if (request.buffer.size() + message_size > client->getBufferLimit())
    // {
    //     HttpResponse response;
    //     response.statusCode = static_cast<HTTP_StatusCode>(413);
    // }
    request.buffer.append(temp_buffer, message_size);
    std::cout << "processClientRequest -> request.buffer: " << request.buffer << "\n";

    

    HttpRequestParser::parse(request);

    if (request.parseResult == PARSE_ERROR)
    {
        std::cout << "PARSE_ERROR" << "\n";
    }
    else if (request.parseResult == PARSE_DONE)
    {
        std::cout << "!!!!!!!!!!!Content type: " << request.contentType << "\n";
        std::cout << "PARSE_DONE" << "\n";
        size_t listen_index = _clientFdToListenerIndex[client_fd];
        Listener& listener = _listeners[listen_index];
        HttpResponse response = _router.handleRequest(request, listener, _clientFdToRemoteAddress[client_fd]);
        //generate response;
        std::string rawResponse = serializeHttpResponse(response);
        
        // ssize_t sent = send(client_fd, rawResponse.c_str(), rawResponse.size(), 0);
        // if (sent < 0)
        // {
        //     std::cerr << "send error on fd " << client_fd << ": " << strerror(errno) << "\n";
        // }
        client->appendToSendBuffer(rawResponse);
        _poll_fds[i].events &= ~POLLIN;
        _poll_fds[i].events |= POLLOUT;

        // close(client_fd);
        // removeClient(client_fd);
        // _poll_fds.erase(_poll_fds.begin() + i);
        // --i;
        // std::cout << "----RAW RESPONSE----\n";
        // std::cout << rawResponse << "\n";
    }
}

void Manager::initializePollFds()
{
    _poll_fds.clear();
    for (size_t i = 0; i < _listeners.size(); i++)
    {
        pollfd p;
        p.fd = _listeners[i].listenFd;
        p.events = POLLIN;
        p.revents = 0;
        _poll_fds.push_back(p);
    }
}

void Manager::closeListenSockets(std::vector<Listener>& listeners)
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

void Manager::receiveDataFromClient(size_t& i)
{
    int client_fd = _poll_fds[i].fd;
    Client* client = getClient(client_fd);
    if (!client)
    {
        _clients[client_fd] = std::make_unique<Client>(client_fd);
        client = _clients[client_fd].get();
        std::cout << "NEW CLIENT created for fd=" << client_fd << "\n";
    }
    else
    {
        std::cout << "REUSING CLIENT for fd=" << client_fd << "\n";
    }
    
    char temp_buffer[4096];
    ssize_t message_size = recv(client_fd, temp_buffer, sizeof(temp_buffer), 0);
    if (message_size == 0)
    {
        // Client disconnected
        close(client_fd);
        removeClient(client_fd);
        _poll_fds.erase(_poll_fds.begin() + i);
        --i;
        return;
    }
    else if (message_size < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // No data available right now, continue
            return;
        }
        std::cerr << "recv error on fd " << client_fd << ": " << strerror(errno) << "\n";
        close(client_fd);
        removeClient(client_fd);
        _poll_fds.erase(_poll_fds.begin() + i);
        --i;
        return;
    }
    else if (client->getBytesReceived() + message_size > _recvBufferSize)
    {
        HttpResponse response;
        response.statusCode = static_cast<HTTP_StatusCode>(413);
        response.headers["Content-Length"] = "0";
        response.headers["Connection"] = "close";
        std::string rawResponse = serializeHttpResponse(response);
        client->appendToSendBuffer(rawResponse);
        _poll_fds[i].events &= ~POLLIN;
        _poll_fds[i].events |= POLLOUT;
        return;
    }
    client->addBytesReceived(message_size);
    processClientRequest(i, temp_buffer, message_size);

}


        // ssize_t sent = send(client_fd, rawResponse.c_str(), rawResponse.size(), 0);
        // if (sent < 0)
        // {
        //     std::cerr << "send error on fd " << client_fd << ": " << strerror(errno) << "\n";
        // }


// SIGCHLD     - Child process ended
// SIGTERM     - Program should terminate  
// SIGPIPE     - Writing to closed pipe/socket
// SIGINT      - Ctrl+C from user

int Manager::run()
{
    initializePollFds();
    while (true)
    {
        // std::cerr << "we started.\n";
        int ready_fds = poll(_poll_fds.data(), _poll_fds.size(), -1);
        if (ready_fds < 0)
        {
            if (errno == EINTR)
                continue;
            std::cerr << "poll failed.\n";
            closeListenSockets(_listeners);
            return -1;
        }
        for (size_t i = 0; i < _poll_fds.size(); i++)
        {
            auto it = _listenFdtoListenerIndex.find(_poll_fds[i].fd);
            bool found = (it != _listenFdtoListenerIndex.end());

            if (_poll_fds[i].revents == 0)
                continue;
                
            if (found && (_poll_fds[i].revents & POLLIN))
            {
                //listening socket ready for reading
                acceptNewConnection(_poll_fds[i].fd, it->second);
            }
            else if (!found && (_poll_fds[i].revents & POLLIN))
            {
                //client sends data, server can read
                receiveDataFromClient(i);
            }
            if (!found && (_poll_fds[i].revents & POLLOUT))
            { 
                //client socket is writable, server can send
                responseToClient(i);
            }
            _poll_fds[i].revents = 0;
        }
    }  
}