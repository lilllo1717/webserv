
#include "Manager.hpp"

Manager::Manager()
    :
        _servers(),
        _clients(),
        _clientFdToServer(),
        _listenFdToServers(),
        _poll_fds()
{
    std::cout << "Manager Constructor called.\n";
}

Manager::~Manager()
{
    std::cout << "Manager Destructor called.\n";
}

void Manager::addServer(std::unique_ptr<Server> server)
{
    int listenFd = server->getListenFd();
    _listenFdToServers[listenFd].push_back(server.get());
    _servers.push_back(std::move(server));
}

std::vector<std::unique_ptr<Server>>& Manager::getServers()
{
    return _servers;
}

// void Manager::run()
// {
//     _poll_fds.clear();

//     for (size_t i = 0; i < _servers.size(); i++)
//     {
//         pollfd p;
//         p.fd = _servers[i]->getListenFd();
//         p.events = POLLIN;
//         p.revents = 0;
//         _poll_fds.push_back(p);
//     }

//     while (true)
//     {
//         int ready_fds = poll(_poll_fds.data(), _poll_fds.size(), -1);

