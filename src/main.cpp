#include "webserv.hpp"


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

int main(int argc, char **argv)
{
	// if (argc != 2)
	// 	return (1);

	(void)argv;
	(void)argc;

	struct sockaddr_in client_address;
	socklen_t	client_len;
	int new_socket_fd;
	char	buffer[256];
	int 	byte_read;

	Server server;
	server.start();

	client_len = sizeof(client_address);
	new_socket_fd = accept(server.getListenFd(), (struct sockaddr*)&client_address, &client_len);
	if (new_socket_fd < 0)
	{
		std::cerr << "accept failed.\n";
		return (1);
	}
	send(new_socket_fd, "Hello, world!\n", 13, 0);
	bzero(buffer, 256);

	byte_read = read(new_socket_fd, buffer, 255);
	printf("Message: %s\n", buffer);
	server.stop();
	return (0);
}