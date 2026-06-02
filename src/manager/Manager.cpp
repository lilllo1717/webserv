
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

std::string getHostNameS(std::string server_host)
{
    auto pos = server_host.find(':');
    if (pos == std::string::npos)
        return server_host;
    return server_host.substr(0, pos);
}

Server* Manager::findServerForRequests(const Listener& listener, const HttpRequest& request)
{
    std::string hostName = getHostNameS(request.host);
    for (const Server* server : listener.servers)
    {
        const std::vector<std::string>& names = server->getServerNames();
        for (const std::string& name : names)
        {
            if (name == hostName)
                return const_cast<Server*>(server);
        }
    }
    return listener.defaultServer;
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
        // std::cout << "DEBUG accept new connection: fd=" << new_socket_fd << "\n";
        if (new_socket_fd < 0)
        {
            int saved_errno = errno;
            if (saved_errno == EAGAIN || saved_errno == EWOULDBLOCK)
                break;
            std::cerr << "accept failed: " << strerror(saved_errno) << "\n";
            break;
        }
        int flags = fcntl(new_socket_fd, F_GETFL, 0);
        if (flags < 0)
        {
            std::cerr << "fcntl F_GETFL failed: " << strerror(errno) << "\n";
            close(new_socket_fd);
            return;
        }
        flags = flags | O_NONBLOCK;
        if (fcntl(new_socket_fd, F_SETFL, flags) < 0)
        {
            std::cerr << "fcntl F_SETFL failed: " << strerror(errno) << "\n";
            close(new_socket_fd);
            return;
        }

        int fdflags = fcntl(new_socket_fd, F_GETFD, 0);
        if (fdflags < 0)
        {
            std::cerr << "fcntl F_GETFD failed: " << strerror(errno) << "\n";
            close(new_socket_fd);
            return;
        }
        fdflags = fdflags | FD_CLOEXEC;
        if (fcntl(new_socket_fd, F_SETFD, fdflags) < 0)
        {
            std::cerr << "fcntl F_SETFD failed: " << strerror(errno) << "\n";
            close(new_socket_fd);
            return;
        }
        addClient(new_socket_fd);
        _clientFdToRemoteAddress[new_socket_fd] = inet_ntoa(client_address.sin_addr);
        std::cout << "DEBUG: New connection from " << _clientFdToRemoteAddress[new_socket_fd] 
                  << " on fd " << new_socket_fd << "\n";
        _clientFdToListenerIndex[new_socket_fd] = listenerIndex;
        std::cout << "DEBUG: Assigned listener index " << listenerIndex << " to client fd " << new_socket_fd << "\n";
        _poll_fds.push_back(pollfd{new_socket_fd, POLLIN, 0});
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
        cleanupClient(client_fd, i);
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
        cleanupClient(client_fd, i);
        return ;
    }

    client->clearBuffer(bytes_sent);

    if (!client->getSendBuffer().empty())
        return;
    
    bool keepAlive = client->getHttpRequest().keepAlive;

    if (keepAlive)
    {
        std::string leftover = client->getHttpRequest().buffer;
        // std::cout << "DEBUG keepAlive leftover size: " << leftover.size() 
        //         << " [" << leftover.substr(0, 50) << "]\n";
        client->getHttpRequest() = HttpRequest();
        _poll_fds[i].events &= ~POLLOUT;
        _poll_fds[i].events |=  POLLIN;

        if (!leftover.empty())
        {
            client->getHttpRequest().buffer = leftover;
            //no new data, process what's in the buffer
            processClientRequest(i, nullptr, 0);
        }
    }
    else
        cleanupClient(client_fd, i); 
}

// void Manager::processClientRequest(size_t& i, std::string temp_buffer, ssize_t message_size)


void Manager::processClientRequest(size_t& i, char* temp_buffer, ssize_t message_size)
{
    int client_fd = _poll_fds[i].fd;
    Client* client = getClient(client_fd);
    if (!client)
        return;
    HttpRequest& request = client->getHttpRequest();
    if (temp_buffer != nullptr)
    {
        request.buffer.append(temp_buffer, message_size);
    }
    ParseResult parseRes = HttpRequestParser::parse(request);
    if (parseRes == PARSE_IN_PROGRESS)
    {
        return;
    }
    if (parseRes == PARSE_ERROR)
    {
        HttpResponse errResp;
        if (request.method == HTTP_UNKNOWN)
        {
            errResp.statusCode = static_cast<HTTP_StatusCode>(405);
        }
        else
        {
            errResp.statusCode = request.uri_too_long
                ? static_cast<HTTP_StatusCode>(414)
                : static_cast<HTTP_StatusCode>(400);
        }
        errResp.headers["Content-Length"] = "0";
        errResp.headers["Connection"] = "close";
        std::string rawResponse = serializeHttpResponse(errResp);
        client->appendToSendBuffer(rawResponse);
        client->getHttpRequest().keepAlive = false;
        _poll_fds[i].events &= ~POLLIN;
        _poll_fds[i].events |= POLLOUT;
    }
    else if (parseRes == PARSE_DONE)
    {
        // std::cout << "!!!!!!!!!!!Content type: " << request.contentType << "\n";
        // std::cout << "PARSE_DONE" << "\n";
        client->resetBytesReceived();
        size_t listen_index = _clientFdToListenerIndex[client_fd];
        Listener& listener = _listeners[listen_index];
        if (request.contentLength > 0)
        {
            Server* server = findServerForRequests(listener, request);
            size_t maxBodySize = server->getConfig().clientMaxBodySize;
            if (maxBodySize > 0 && request.contentLength > maxBodySize)
            {
                HttpResponse response;
                response.statusCode = static_cast<HTTP_StatusCode>(413);
                response.headers["Content-Length"] = "0";
                response.headers["Connection"] = "close";
                client->appendToSendBuffer(serializeHttpResponse(response));
                client->getHttpRequest().keepAlive = false;
                _poll_fds[i].events &= ~POLLIN;
                _poll_fds[i].events |= POLLOUT;
                return;
            }
        }
        RouterResult result = _router.handleRequest(request, listener, _clientFdToRemoteAddress[client_fd]);
        if (result.decision == DES_ERROR)
        {
            HttpResponse errResp = RequestHandler::buildErrorResponse(result.response.statusCode, result.matchResult.selectedServerCon);
            result.response.closeConnection = !request.keepAlive;
            client->appendToSendBuffer(serializeHttpResponse(errResp));
            _poll_fds[i].events &= ~POLLIN;
            _poll_fds[i].events |= POLLOUT;
        }
        else if (result.decision == DES_NORMAL)
        {
            HttpResponse response = RequestHandler::executeNormal(request, *result.routeConfigure, result.matchResult.selectedServerCon);
            response.closeConnection = !request.keepAlive;
            client->appendToSendBuffer(serializeHttpResponse(response));
            _poll_fds[i].events &= ~POLLIN;
            _poll_fds[i].events |= POLLOUT;
        }
        
        else if (result.decision == DES_REDIRECT)
        {
            result.response.closeConnection = !request.keepAlive;
            client->appendToSendBuffer(serializeHttpResponse(result.response));
            _poll_fds[i].events &= ~POLLIN;
            _poll_fds[i].events |= POLLOUT;
        }
        else if (result.decision == DES_CGI)
        {
            CgiHandler cgi(request, *result.routeConfigure, result.matchResult);
            CgiState state;
            if (!cgi.executeCgi(state))
            {
                HttpResponse response = RequestHandler::buildErrorResponse(
                    static_cast<HTTP_StatusCode>(500),
                    result.matchResult.selectedServerCon);
                response.closeConnection = !request.keepAlive;
                client->appendToSendBuffer(serializeHttpResponse(response));
                _poll_fds[i].events &= ~POLLIN;
                _poll_fds[i].events |= POLLOUT;
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

void Manager::cleanupClient(int client_fd, size_t& i)
{

    close(client_fd);
    removeClient(client_fd);
    _poll_fds.erase(_poll_fds.begin() + i);
    --i;
}

void Manager::receiveDataFromClient(size_t& i)
{
    int client_fd = _poll_fds[i].fd;
    Client* client = getClient(client_fd);
    if (!client)
    {
        cleanupClient(client_fd, i);
        return;
    }
    // Server* server = getServerByClientFd(client_fd);
    char temp_buffer[4096];
    ssize_t message_size = recv(client_fd, temp_buffer, sizeof(temp_buffer), 0);
    if (message_size == 0)
    {
        // Client disconnected
        cleanupClient(client_fd, i);
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
        cleanupClient(client_fd, i);
        return;
    }
    else if (client->getBytesReceived() + static_cast<size_t>(message_size) > _recvBufferSize)
    {
        HttpResponse response;
        response.statusCode = static_cast<HTTP_StatusCode>(413);
        response.headers["Content-Length"] = "0";
        response.headers["Connection"] = "close";
        std::string rawResponse = serializeHttpResponse(response);
        client->appendToSendBuffer(rawResponse);
        client->getHttpRequest().keepAlive = false;
        _poll_fds[i].events &= ~POLLIN;
        _poll_fds[i].events |= POLLOUT;
        return;
    }
    client->addBytesReceived(message_size);
    processClientRequest(i, temp_buffer, message_size);
}


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
            std::cerr << "poll failed: " << strerror(errno) << "\n";
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
                    cleanupClient(_poll_fds[i].fd, i);
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
