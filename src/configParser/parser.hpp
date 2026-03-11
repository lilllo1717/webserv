#ifndef PARSER_HPP
# define PARSER_HPP

# include <algorithm>
# include <map>
# include <string>
# include <vector>

# include "tokenizer.hpp"


// Specific URL path inside that website
struct routeConfig 
{
	std::string					path;
	std::vector<std::string>	httpMethods;	// GET, POST, and DELETE
	std::string					rootDir; // directory to serve
	std::string					index; // default file

	bool						autoindex = false;

	bool						isRedirect = false;
	int							redirectCode = 0;
	std::string					redirectTarget;

	bool						allow_upload = true; // turns on and off the upload feature
	std::string					uploadPath; // where to store uploaded files

	std::map<std::string, std::string>	cgi;
};

// struct listenConfig
// {
// 	std::string	host;
// 	int			port;
// };

struct serverEndpoint
{
	std::string					ip;
	int							port;
};

// Apply to the whole virtual host/website
struct serverConfig
{
	serverEndpoint					endpoint; // ip + port to listen on
	// std::vector<serverEndpoint>	listen; // IP address of server
	std::vector<std::string>	serverNames; // list of domains/urls of website
	std::map<int, std::string>	errorPages; // 404, 400, 500, etc.
	size_t						clientMaxBodySize = 0;
	std::vector<routeConfig>	routes; // routes inside this server
};

// Puts everything together into a vector of server structs
struct mainConfig
{
	std::vector<serverConfig> servers;
};

// create seperate struct for 

class Parser
{
	private:
		std::vector<Token>	_tokens; // vector container of token structs
		size_t				_position; // position of token in vector of tokens

		// traversing through vector of tokens with some verification
		const Token&	currentPosition() const;
		bool			checkifEOF();
		const Token&	moveForward();
		const Token&	verifyToken(tokenType tok, const std::string& errorMessage);
		const Token& 	compareWord(const std::string& word, const std::string& msg);
	
		void	throwError(const std::string& message) const;

		// parse client max body size based on suffixes (K, M, G)
		size_t			convertClientMaxBodySize(const std::string& nb);

		// parse location block inside server block
		void			parseMethodsDirective(routeConfig& rC);
		void			parseRootDirective(routeConfig& rC);
		void			parseIndexDirective(routeConfig& rC);
		void			parseAutoindexDirective(routeConfig& rC);
		void			parseUploadStoreDirective(routeConfig& rC);
		void			parseCGIDirective(routeConfig& rC);
		void			parseReturnDirective(routeConfig& rC);

		void			parseInsideLocationBlock(routeConfig& rC);
		routeConfig		parseLocationBlock();

		// parse server block
		void			isValidPort(int port);
		void			isValidIP(const std::string& ip);

		void			isValidStatusCode(int code, bool directive);
		
		void			parseListenDirective(serverConfig& sC);
		void			parseServerNameDirective(serverConfig& sC);
		void			parseBodySizeDirective(serverConfig& sC);
		void			parseErrorPageDirective(serverConfig& sC);

		void			parseInsideServerBlock(serverConfig& sC);
		serverConfig	parseServerBlock();

	public:
		Parser(): _position(0) {}
		Parser(const std::vector<Token>& tokens): _tokens(tokens), _position(0) {}
		// Parser(const Parser& other);
		// ~Parser();

		mainConfig	parse();

};

#endif
