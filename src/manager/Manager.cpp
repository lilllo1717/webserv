
#include "Manager.hpp"
#include "../../include/webserv.hpp"


Manager::Manager()
    :
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

void Manager::startListenersServers()
{
    startListeners(_listeners);
    setListeners();
}

std::vector<Listener>& Manager::getListeners()
{
    
    return _listeners;

}


void Manager::run()
{
    _poll_fds.clear();

    struct sockaddr_in client_address;
	socklen_t	client_len;
	int new_socket_fd;

    for (size_t i = 0; i < _listeners.size(); i++)
    {
        pollfd p;
        p.fd = _listeners[i].listenFd;
        p.events = POLLIN;
        p.revents = 0;
        _poll_fds.push_back(p);
    }

    while (true)
    {
        // std::cerr << "we started.\n";
        int ready_fds = poll(_poll_fds.data(), _poll_fds.size(), -1);
        if (ready_fds < 0)
        {
            std::cerr << "poll failed.\n";
            // close(_listenFd);
            // _started = false;
            break ;
        }
        for (size_t i = 0; i < _poll_fds.size(); i++)
        {
            std::cout << "entered loop." << "\n";
            std::cout << "_poll_fds[i].fd: " << _poll_fds[i].fd << "\n";

            auto it = _listenFdtoListenerIndex.find(_poll_fds[i].fd);
            bool found = (it != _listenFdtoListenerIndex.end());
            std::cout << "found: " << found << "\n";


            if (_poll_fds[i].revents == 0)
                continue;

            if (found && (_poll_fds[i].revents & POLLIN))
            {
                while (true)
                {
                    client_len = sizeof(client_address);
                    new_socket_fd = accept(_poll_fds[i].fd, (struct sockaddr*)&client_address, &client_len);
                    _clientFdToRemoteAddress[new_socket_fd] = inet_ntoa(client_address.sin_addr);
                    if (new_socket_fd < 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break;
                        // _started = false;
                        break;
                    }
                    // int flags = fcntl(new_socket_fd, F_GETFL, 0);
                    // fcntl(new_socket_fd, F_SETFL, flags | O_NONBLOCK);

                    // int fdflags = fcntl(new_socket_fd, F_GETFD, 0);
                    // fcntl(new_socket_fd, F_SETFD, fdflags | FD_CLOEXEC);
                    size_t listenerIndex = it->second;
                    _poll_fds.push_back(pollfd{new_socket_fd, POLLIN, 0});
                    addClient(new_socket_fd);
                    _clientFdToListenerIndex[new_socket_fd] = listenerIndex;
                }
            }
            else if (!found && (_poll_fds[i].revents & POLLIN))
            {
                char temp_buffer[1024];
                ssize_t message_size = recv(_poll_fds[i].fd, temp_buffer, sizeof(temp_buffer), 0);
                int client_fd = _poll_fds[i].fd;
                
                if (message_size == 0)
                {
                    // Client disconnected
                    close(client_fd);
                    removeClient(client_fd);
                    _poll_fds.erase(_poll_fds.begin() + i);
                    --i;
                    continue;
                }
                else if (message_size < 0)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        // No data available right now, continue
                        continue;
                    }
                    std::cerr << "recv error on fd " << client_fd << ": " << strerror(errno) << "\n";
                    close(client_fd);
                    removeClient(client_fd);
                    _poll_fds.erase(_poll_fds.begin() + i);
                    --i;
                    continue;
                }
                std::cerr << "DBG recv fd=" << client_fd
                << " bytes=" << message_size
                << " data=[" << std::string(temp_buffer, message_size) << "]\n";


                Client* client = getClient(client_fd);
                if (!client)
                    continue;
                HttpRequest& request = client->getHttpRequest();
                request.buffer.append(temp_buffer, message_size);
                HttpRequestParser::parse(request);
                
                std::cout << "Parsed request method: " << request.method << "\n";
                // client->appendToReceiveBuffer(std::string(temp_buffer, message_size));
                // client->appendToSendBuffer(std::string(temp_buffer, message_size));
                // _poll_fds[i].events |= POLLOUT;
                // addBytesReceived(message_size);
                std::cout << "Received " << message_size << " bytes from client " << client_fd << "message: " << request.buffer << "\n";
                std::cout << "ParseResut: " << request.parseResult << "\n";
                if (request.parseResult == PARSE_ERROR)
                {
                    std::cout << "PARSE_ERROR" << "\n";
                }
                else if (request.parseResult == PARSE_DONE)
                {
                    std::cout << "PARSE_DONE" << "\n";
                    size_t listen_index = _clientFdToListenerIndex[client_fd];
                    std::cout << "listen_index: " << listen_index << "\n";

                    Listener& listener = _listeners[listen_index];
                    std::cout << "listener: " << listener.endpoint.ip << " " << listener.endpoint.port << "\n";

                    HttpResponse response = _router.handleRequest(request, listener, _clientFdToRemoteAddress[client_fd]);
                    //generate response;
					std::string rawResponse = serializeHttpResponse(response);
					
					// DEBUG: To check if serialize function works
					std::cout << "----RAW RESPONSE----\n";
					std::cout << rawResponse << "\n";
                }

            }

            if (!found && (_poll_fds[i].revents & POLLOUT))
            { 
                int client_fd = _poll_fds[i].fd;
                Client* client = getClient(client_fd);
                if (!client)
                    continue;
                const std::string& sendBuffer = client->getSendBuffer();
                if (sendBuffer.empty())
                {
                    _poll_fds[i].events &= ~POLLOUT;
                    continue;
                }
                ssize_t bytes_sent = send(client_fd, sendBuffer.c_str(), sendBuffer.size(), 0);
                if (bytes_sent < 0)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        continue;
                    }
                    std::cerr << "send error on fd " << client_fd << ": " << strerror(errno) << "\n";
                    close(client_fd);
                    removeClient(client_fd);
                    _poll_fds.erase(_poll_fds.begin() + i);
                    --i;
                    continue;
                }
                // Remove sent bytes from the send temp_buffer
                client->clearBuffer(bytes_sent);
                // addBytesSent(bytes_sent);
                std::cout << "Sent " << bytes_sent << " bytes to client " << client_fd << "\n";
            }
            _poll_fds[i].revents = 0;
        }
    }
    
}