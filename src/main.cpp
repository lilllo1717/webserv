#include "webserv.hpp"
# include "../src/client/Client.hpp"
# include "../src/server/Server.hpp"
# include "../src/manager/Manager.hpp"


// Create one listening socket
// Accept one client
// Read their HTTP request
// Send back a hard-coded HTTP response
// Close everything and exit

// A server is a program that:
// Creates a communication endpoint
// Announces “I am accepting connections on this port”
// Waits for someone to connect
// Once connected, exchanges data with the client
// Closes the connection

// A client is a program that:
// Creates its own communication endpoint
// Chooses a server’s IP + port
// Requests a connection
// Sends data to that server
// Optionally waits for a response



void initializeConfig(serverConfig& config, const std::string& ip, int port)
{
	config.endpoint.ip = ip;
	config.endpoint.port = port;
}



int main(int argc, char **argv)
{
	// if (argc != 2)
	// 	return (1);
	// std::vector<pollfd> poll_fds;

	(void)argv;
	(void)argc;

	// struct sockaddr_in client_address;
	// socklen_t	client_len;
	// int new_socket_fd;
	// char	buffer[256];
	// int 	byte_read;

	Manager manager;
	std::vector<serverConfig> configs;
	// std::vector<Listener> listeners;

	serverConfig config1;
	serverConfig config2;
	serverConfig config3;

	initializeConfig(config1, "127.0.0.1", 8080);
	initializeConfig(config2, "127.0.0.1", 8080);
	initializeConfig(config3, "127.0.0.2", 8081);

	configs.push_back(config1);
	configs.push_back(config2);
	configs.push_back(config3);

	for (size_t i = 0; i < configs.size(); i++)
	{
		manager.addServer(std::make_unique<Server>(configs[i]));
	}
	manager.buildListenersFromServers();
	manager.startListenersServers();
	manager.run();
	// for (size_t i = 0; i < listeners.size(); i++)
	// {
	// 	std::cout << " listeners: "<< listeners[i].endpoint.ip << "\n";
	// }

	// server.start();

	// client_len = sizeof(client_address);
	// new_socket_fd = accept(server.getListenFd(), (struct sockaddr*)&client_address, &client_len);
	// if (new_socket_fd < 0)
	// {
	// 	std::cerr << "accept failed.\n";
	// 	return (1);
	// }
	// send(new_socket_fd, "Hello, world!\n", 13, 0);
	// bzero(buffer, 256);

	// byte_read = read(new_socket_fd, buffer, 255);
	// printf("Message: %s\n", buffer);
	// server.run();
	// server.stop();
	return (0);
}