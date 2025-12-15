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

// Apply to the whole virtual host/website
struct serverConfig
{
	std::vector<std::string>	listen; // IP address of server
	std::vector<std::string>	serverNames; // list of domains/urls of website
	std::vector<std::string>	errorPages; // 404, 400, 500, etc.
	std::vector<routeConfig>	routes; // routes inside this server
};

#endif