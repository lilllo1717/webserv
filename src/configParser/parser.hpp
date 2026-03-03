#ifndef PARSER_HPP
# define PARSER_HPP

# include <vector>
# include <string>


// Specific URL path inside that website
struct routeConfig 
{
	std::string					path;
	std::vector<std::string>	httpMethods;	// GET, POST, and DELETE
	std::string					rootDir; // directory to serve
	std::string					defaultFile; // default fize
	bool						allow_upload = true; // turns on and off the upload feature
	std::string					uploadPath; // where to store uploaded files
	std::string					cgiExt;	// ".php" in our case
	std::string					cgiPath;	// ".php" file path
};

struct serverEndpoint
{
	std::string					ip;
	int							port;
};

// Apply to the whole virtual host/website
struct serverConfig
{
	serverEndpoint					endpoint; // ip + port to listen on
	std::vector<std::string>	serverNames; // list of domains/urls of website
	std::vector<std::string>	errorPages; // 404, 400, 500, etc.
	std::vector<routeConfig>	routes; // routes inside this server
};

#endif

