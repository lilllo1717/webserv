#include "webserv.hpp"
# include "../src/configParser/parser.hpp"
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

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: ./webserv <config_file>\n";
        return 1;
    }

    try
    {
        // Read config file
        std::string configText = readFile(argv[1]);

        // Tokenize
        Tokenizer tokenizer(configText);
        std::vector<Token> tokens;

        Token token;
        do
        {
            token = tokenizer.createToken();
            tokens.push_back(token);
        }
        while (token.type != tokenType::TOKEN_EOF);

        // Parse
        Parser parser(tokens);
        mainConfig parsedConfig = parser.parse();

        // Create Manager
        Manager manager;

        // Add parsed servers to manager
        for (size_t i = 0; i < parsedConfig.servers.size(); ++i)
        {
            manager.addServer(std::make_unique<Server>(parsedConfig.servers[i]));
            std::cout << "\n Added server with port " << parsedConfig.servers[i].endpoint.port << "\n";
            std::cout << "Added server with host " << parsedConfig.servers[i].endpoint.ip << "\n";
            std::cout << "Added server with " << parsedConfig.servers[i].serverNames[0] << " server names\n";
        }
        manager.buildListenersFromServers();
        if (manager.startListenersServers() == -1)
            return 1;
        if (manager.run() == -1)
            return 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
