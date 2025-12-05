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


	struct sockaddr_in server_address, client_address;
	socklen_t	client_len;
	int new_socket_fd;
	char	buffer[256];
	int 	byte_read;
		
	int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd< 0)
	{
		std::cerr << "Socket creation failed.\n";
		return (1);
	}
	int opt = 1;
	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		std::cerr << "setsockopt failed.\n";
		return (1);
	}
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(8080);

	if (bind(socket_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0)
	{
		std::cerr << "bind failed.\n";
		return (1);
	}

	if (listen(socket_fd, 10) < 0)
	{
		std::cerr << "listen to socket failed.\n";
		return (1);
	}

	client_len = sizeof(client_address);
	new_socket_fd = accept(socket_fd, (struct sockaddr*)&client_address, &client_len);
	if (new_socket_fd < 0)
	{
		std::cerr << "accept failed.\n";
		return (1);
	}
	send(new_socket_fd, "Hello, world!/n", 13, 0);
	bzero(buffer, 256);

	byte_read = read(new_socket_fd, buffer, 255);
	printf("Message: %s\n", buffer);

	close(new_socket_fd);
	close(socket_fd);

	return (0);

}