
#include "Manager.hpp"
#include "../../include/webserv.hpp"


Manager::Manager()
    :
        _recvBufferSize(100000),
        _maxConnections(128),
        _servers(),
        _clients(),
        _clientFdToServer(),
        // _listenFdToServers(),
        _poll_fds(),
        _listeners(),
        _listenFdtoListenerIndex(),
        _clientFdToRemoteAddress(),
        _cgiFdtoClientFd()
    
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
    _clientFdToListenerIndex.erase(socketFd);
    _clientFdToRemoteAddress.erase(socketFd);
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


void Manager::finalizeCgiOutput(int client_fd, Client* client)
{
    CgiState* cgiState = client->getCgiState();
    if (!cgiState)
        return;
    waitpid(cgiState->childPid, NULL, WNOHANG);
    HttpResponse response = CgiHandler::parseCgiOutputIntoHttpResponse(cgiState->output);
    client->appendToSendBuffer(serializeHttpResponse(response));
    client->cleanupCgiState();
    for (size_t i = 0; i < _poll_fds.size(); i++)
    {
        if (_poll_fds[i].fd == client_fd)
        {
            _poll_fds[i].events &= ~POLLIN;
            _poll_fds[i].events |=  POLLOUT;
            break;
        }
    }
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

void Manager::writeCgiBody(size_t& i, Client* client)
{
    CgiState* cgiState = client->getCgiState();
    if (!cgiState || cgiState->stdInFd == -1)
        return;
    
    const std::vector<uint8_t>& body = client->getHttpRequest().body;
    const uint8_t* bodyData = body.data() + cgiState->bodyWritten;
    size_t bodyLeft = body.size() - cgiState->bodyWritten;

    ssize_t bytes_written = write(cgiState->stdInFd, bodyData, bodyLeft);
    if (bytes_written > 0)
        cgiState->bodyWritten += static_cast<size_t>(bytes_written);
    
    bool wouldBlock = (bytes_written < 0) && (errno == EAGAIN || errno == EWOULDBLOCK);
    bool error = (bytes_written == 0) || (bytes_written < 0 && !wouldBlock);
    bool done = (cgiState->bodyWritten >= body.size());

    if (error || done)
    {
        close(cgiState->stdInFd);
        _cgiFdtoClientFd.erase(cgiState->stdInFd);
        _poll_fds.erase(_poll_fds.begin() + i);
        cgiState->stdInFd = -1;
        cgiState->stdInDone = true;
        --i;
    }
}

void Manager::readCgiOutput(size_t& i, Client* client)
{
    CgiState* cgiState = client->getCgiState();
    if (!cgiState || cgiState->stdoutFd == -1)
        return;
    
    char temp_buffer[4096];
    ssize_t bytes_read = read(cgiState->stdoutFd, temp_buffer, sizeof(temp_buffer));
    if (bytes_read > 0)
    {
        cgiState->output.append(temp_buffer, bytes_read);
        return;
    }
    
    bool error = (bytes_read < 0) && (errno != EAGAIN && errno != EWOULDBLOCK);
    bool done = (bytes_read == 0);

    if (error || done)
    {
        close(cgiState->stdoutFd);
        _cgiFdtoClientFd.erase(cgiState->stdoutFd);
        _poll_fds.erase(_poll_fds.begin() + i);
        cgiState->stdoutFd = -1;
        cgiState->done = true;
        --i;

        finalizeCgiOutput(client->getSocketFd(), client);
    }
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

    if (!client->getSendBuffer().empty())
        return;
    
    bool keepAlive = client->getHttpRequest().keepAlive;

    if (keepAlive)
    {
        client->getHttpRequest() = HttpRequest();
        _poll_fds[i].events &= ~POLLOUT;
        _poll_fds[i].events |=  POLLIN;
    }

    else
    {
        close(client_fd);
        removeClient(client_fd);
        _poll_fds.erase(_poll_fds.begin() + i);
        --i;
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

    // if (request.parseResult == PARSE_ERROR)
    // {
    //     std::cout << "PARSE_ERROR" << "\n";
    //     HttpResponse errResp;
    //     errResp.statusCode = static_cast<HTTP_StatusCode>(400);
    //     errResp.headers["Content-Length"] = "0";
    //     errResp.headers["Connection"] = "close";
    //     client->appendToSendBuffer(serializeHttpResponse(errResp));
    //     _poll_fds[i].events &= ~POLLIN;
    //     _poll_fds[i].events |= POLLOUT;
    // }
    if (request.parseResult == PARSE_ERROR)
    {
        HttpResponse errResp;
        // 414 if URI was too long, 400 otherwise
        errResp.statusCode = (request.parseState == ERROR)
            ? static_cast<HTTP_StatusCode>(414)
            : static_cast<HTTP_StatusCode>(400);
        errResp.headers["Content-Length"] = "0";
        errResp.headers["Connection"] = "close";
        client->appendToSendBuffer(serializeHttpResponse(errResp));
        _poll_fds[i].events = POLLOUT;
    }
    else if (request.parseResult == PARSE_DONE)
    {
        // std::cout << "!!!!!!!!!!!Content type: " << request.contentType << "\n";
        // std::cout << "PARSE_DONE" << "\n";
        size_t listen_index = _clientFdToListenerIndex[client_fd];
        Listener& listener = _listeners[listen_index];
        RouterResult result = _router.handleRequest(request, listener, _clientFdToRemoteAddress[client_fd]);
        //generate response;

        if (result.decision == DES_ERROR)
        {
            result.response.closeConnection = !request.keepAlive;
            client->appendToSendBuffer(serializeHttpResponse(result.response));
            _poll_fds[i].events = POLLOUT;
        }
        else if (result.decision == DES_NORMAL)
        {
            HttpResponse response = RequestHandler::executeNormal(request, *result.routeConfigure, result.matchResult.selectedServerCon);
            response.closeConnection = !request.keepAlive;
            client->appendToSendBuffer(serializeHttpResponse(response));
            _poll_fds[i].events = POLLOUT;
        }
        else if (result.decision == DES_REDIRECT)
        {
            result.response.closeConnection = !request.keepAlive;
            client->appendToSendBuffer(serializeHttpResponse(result.response));
            _poll_fds[i].events = POLLOUT;
        }
        else if (result.decision == DES_CGI)
        {
            CgiHandler cgi(request, *result.routeConfigure, result.matchResult);
            CgiState state;
            if (!cgi.executeCgi(state))
            {
                HttpResponse response;
                response.statusCode = static_cast<HTTP_StatusCode>(500);
                client->appendToSendBuffer(serializeHttpResponse(response));
                _poll_fds[i].events = POLLOUT;
                return;
            }
            client->initializeCgiState(state);
            if (state.stdInFd != -1)
            {
                _cgiFdtoClientFd[state.stdInFd] = client_fd;
                _poll_fds.push_back(pollfd{state.stdInFd, POLLOUT, 0});
            }
            _poll_fds.push_back(pollfd{state.stdoutFd, POLLIN, 0});
            _cgiFdtoClientFd[state.stdoutFd] = client_fd;

            _poll_fds[i].events = POLLHUP;
        }
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
        close(client_fd);
        _poll_fds.erase(_poll_fds.begin() + i);
        --i;
        return;
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
    else if (static_cast<size_t>(client->getBytesReceived()) + static_cast<size_t>(message_size) > _recvBufferSize)
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
            if (_poll_fds[i].revents == 0)
                continue;
            int fd = _poll_fds[i].fd;
            auto listerner_it = _listenFdtoListenerIndex.find(fd);
            auto cgi_it = _cgiFdtoClientFd.find(fd);

            auto isListenerFd = (listerner_it != _listenFdtoListenerIndex.end());
            auto isCgiFd = (cgi_it != _cgiFdtoClientFd.end());

            if (isListenerFd && (_poll_fds[i].revents & POLLIN))
            {
                //listening socket ready for reading
                acceptNewConnection(_poll_fds[i].fd, listerner_it->second);
            }
            else if (isCgiFd)
            {
                int client_fd = cgi_it->second;
                Client* client = getClient(client_fd);
                if(!client)
                {
                    close(fd);
                    _cgiFdtoClientFd.erase(fd);
                    _poll_fds.erase(_poll_fds.begin() + i);
                    --i;
                    continue;
                }
                if (_poll_fds[i].revents & POLLOUT)
                    writeCgiBody(i, client);
                else if (_poll_fds[i].revents & (POLLIN | POLLHUP))
                    readCgiOutput(i, client);
            }
            else
            {
                if (_poll_fds[i].revents & (POLLHUP | POLLERR))
                {
                    close(fd);
                    removeClient(fd);
                    _poll_fds.erase(_poll_fds.begin() + i);
                    --i;
                    continue;
                }
                if (_poll_fds[i].revents & POLLIN)
                {
                    receiveDataFromClient(i);
                }
                if (i < _poll_fds.size() && (_poll_fds[i].revents & POLLOUT))
                {
                    responseToClient(i);
                }
            }
            if (i < _poll_fds.size())
                _poll_fds[i].revents = 0;
        }
    }  
}