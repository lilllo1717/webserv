#include "parser.hpp"

// Traversing through the vector of tokens and doing some verification

const Token&	Parser::currentPosition() const
{
	if (_position >= _tokens.size())
	{
		static Token eof(tokenType::TOKEN_EOF, "", 1, 1);
		return eof;
	}
	return _tokens[_position];
}

bool	Parser::checkifEOF()
{
	return currentPosition().type == tokenType::TOKEN_EOF;
}

const Token&	Parser::moveForward()
{
	if (!checkifEOF())
		_position++;
	return currentPosition();
}

void	Parser::throwError(const std::string& message) const
{
	std::ostringstream oss;
	oss << "Error at " << currentPosition().line << ":" << currentPosition().column
		<< ": " << message;
	throw std::runtime_error(oss.str());
}

const Token&	Parser::verifyToken(tokenType tok, const std::string& errorMessage)
{
	if (currentPosition().type == tokenType::TOKEN_ERROR)
		throwError("Error with verifying token: " + currentPosition().value);
	if (currentPosition().type != tok)
		throwError(errorMessage);
	
	const Token&	token = currentPosition();
	moveForward();
	return token;
}

const Token& Parser::compareWord(const std::string& word, const std::string& message)
{
    const Token& token = verifyToken(tokenType::TOKEN_WORD, message);
    if (token.value != word)
	{
        std::ostringstream oss;
        oss << message << " (got '" << token.value << "')";
		throwError(oss.str());
    }
    return token;
}

size_t	Parser::convertClientMaxBodySize(const std::string& nb)
{
	if (nb.empty())
		throw std::runtime_error("Empty input");
	
	// Iterate until the suffix part of the string
	size_t i = 0;
	while (i < nb.size() && std::isdigit(static_cast<unsigned char>(nb[i])))
		i++;
	if (i == 0)
		throw std::runtime_error("Size is 0: Invalid");
	
	// Convert the digit part into a number data type
	size_t	convertedNb = 0;
	size_t	j = 0;
	while (j < i)
	{
		int digit = nb[j] - '0';

		if (convertedNb > (std::numeric_limits<size_t>::max() - digit) / 10)
			throw std::runtime_error("Body size overflow");

		convertedNb = convertedNb * 10 + digit;
		j++;
	}
	if (i == nb.size())
		return convertedNb;
	if (i + 1 != nb.size())
		throw std::runtime_error("Invalid suffix format");
	
	char	suffix = nb[i];
	size_t	multiplier = 1;

	if (suffix == 'K')
		multiplier = 1024ULL;
	if (suffix == 'M')
		multiplier = 1024ULL * 1024ULL;
	if (suffix == 'G')
		multiplier = 1024ULL * 1024ULL * 1024ULL;
	
	if (convertedNb > std::numeric_limits<size_t>::max() / multiplier)
		throw std::runtime_error("Body size overflow");
	
	return convertedNb * multiplier;
	
	throw std::runtime_error("Unknown suffix specified");
}

// Parsing of location block (starting from the inside out)

static bool	isValidMethod(const std::string& method)
{
	return (method == "GET" || method == "POST" || method == "DELETE");
}

void	Parser::parseMethodsDirective(routeConfig& rC)
{
	if (rC.methodsSet)
		throwError("Duplicate methods directive");

	rC.methodsSet = true;

	if (currentPosition().type != tokenType::TOKEN_WORD)
		throwError("There should be at least one HTTP method after 'methods'");

	bool	parsedAtLeastOne = false;

	while (currentPosition().type == tokenType::TOKEN_WORD
		&& isValidMethod(currentPosition().value))
	{
		const std::string&	value = currentPosition().value;

		if (std::find(rC.httpMethods.begin(), rC.httpMethods.end(), value) != rC.httpMethods.end())
			throwError("Duplicate HTTP method");

		rC.httpMethods.push_back(value);
		moveForward();
		parsedAtLeastOne = true;
	}
	if (!parsedAtLeastOne)
			throwError("Invalid methods: only 'GET', 'POST', 'DELETE' are allowed");
			
	verifyToken(tokenType::TOKEN_SEMICOLON, "Expected ';' after methods value");
}

void	Parser::parseRootDirective(routeConfig& rC)
{
	if (rC.rootSet)
		throwError("Duplicate root directive");

	rC.rootSet = true;

	const Token&	root = verifyToken(tokenType::TOKEN_WORD, "Expected route path after 'root' directive");
	rC.rootDir = root.value;
	verifyToken(tokenType::TOKEN_SEMICOLON, "Expected ';' after root value");
}

void	Parser::parseIndexDirective(routeConfig& rC)
{
	if (rC.indexSet)
		throwError("Duplicate index directive");
	
	rC.indexSet = true;

	const Token&	index = verifyToken(tokenType::TOKEN_WORD, "Expected index value after 'index' directive");
	rC.index = index.value;
	verifyToken(tokenType::TOKEN_SEMICOLON, "Expected ';' after index value");
}

void	Parser::parseAutoindexDirective(routeConfig& rC)
{
	if (rC.autoindexSet)
		throwError("Duplicate autoindex directive");

	rC.autoindexSet = true;

	const Token&	autoindex = verifyToken(tokenType::TOKEN_WORD, "Expected autoindex value after 'autoindex' directive");

	if (autoindex.value == "on")
		rC.autoindex = true;
	else if (autoindex.value == "off")
		rC.autoindex = false;
	else
		throwError("Invalid autoindex value");
	
	verifyToken(tokenType::TOKEN_SEMICOLON, "Expected ';' after autoindex value");
}

void	Parser::parseUploadStoreDirective(routeConfig& rC)
{
	if (rC.uploadStoreSet)
		throwError("Duplicate upload_store directive");
	
	rC.uploadStoreSet = true;

	rC.allow_upload = true;
	const Token&	upload_store = verifyToken(tokenType::TOKEN_WORD, "Expected upload path after 'upload_store' directive");
	rC.uploadPath = upload_store.value;
	verifyToken(tokenType::TOKEN_SEMICOLON, "Expected ';' after upload_store value");
}

static bool	isValidFileExtenstion(const std::string& file)
{
	if (file.empty() || file[0] != '.')
		return false;
	
	if (file.size() == 1)
		return false;

	for (size_t i = 1; i < file.size(); i++)
	{
		if (!std::isalnum(static_cast<unsigned char>(file[i])))
			return false;
	}
	return true;
}

void	Parser::parseCGIDirective(routeConfig& rC)
{
	if (currentPosition().type != tokenType::TOKEN_WORD)
		throwError("There should be a CGI extension after 'cgi' directive");

	std::string	extension = currentPosition().value;

	if (!isValidFileExtenstion(extension))
    	throwError("Invalid CGI extension format (must start with '.')");

	moveForward();

	if (currentPosition().type != tokenType::TOKEN_WORD)
		throwError("There should be a CGI path after extension");

	std::string executable = currentPosition().value;
	moveForward();

	if (rC.cgi.find(extension) != rC.cgi.end())
		throwError("Duplicate CGI extension: " + extension);

	rC.cgi[extension] = executable;

	verifyToken(tokenType::TOKEN_SEMICOLON, "Expected ';' after cgi values");
}

void	Parser::isValidStatusCode(int code, StatusCodeMode mode)
{
	if (code < 100 || code > 599)
		throwError("Invalid HTTP status code");
	
	if (mode == StatusCodeMode::Any)
		return;
	
	if (mode == StatusCodeMode::ErrorPage)
	{
		if (code < 300)
			throwError("Invalid error_page status code");
		return;
	}

	if (mode == StatusCodeMode::Redirect)
	{
		if (code != 301 && code != 302 && code != 303 && code != 307 && code != 308)
			throwError("Invalid redirect status code");
	}
}

static bool isValidRedirectTarget(const std::string& target)
{
	if (target.empty())
		return false;

	if (target[0] == '/')
		return true;

	if (target.compare(0, 7, "http://") == 0)
		return true;

	if (target.compare(0, 8, "http://") == 0)
		return true;
	
	return false;
}

void	Parser::parseReturnDirective(routeConfig& rC)
{
	if (rC.isRedirect)
		throwError("Duplicate return directive");

	rC.isRedirect = true;

	if (currentPosition().type != tokenType::TOKEN_WORD || currentPosition().value.empty()
		|| !std::all_of(currentPosition().value.begin(), currentPosition().value.end(), ::isdigit))
	{
		throwError("There should be a return code after 'return' directive");
	}

	const std::string& value = currentPosition().value;
	int code = std::stoi(value);
	isValidStatusCode(code, StatusCodeMode::Redirect);

	rC.redirectCode = code;
	moveForward();

	const Token&	redirectPath = verifyToken(tokenType::TOKEN_WORD, "Expected redirect target after error code");

	if (!isValidRedirectTarget(redirectPath.value))
		throwError("Redirect target must be a path or URL");
	
	rC.redirectTarget = redirectPath.value;
	verifyToken(tokenType::TOKEN_SEMICOLON, "Expected ';' after return value");
}

void	Parser::parseBodySizeDirective(routeConfig& sC)
{
	if (sC.bodySizeSet)
		throwError("Duplicate client_max_body_size directive");
	
	sC.bodySizeSet = true;

	const Token&	bodySize = verifyToken(tokenType::TOKEN_WORD, "Expected number size after 'client_max_body_size' directive");
	
	try
	{
		sC.clientMaxBodySize = convertClientMaxBodySize(bodySize.value);
	}
	catch(const std::exception& e)
	{
		throwError("Invalid client_max_body_size value");
	}
	
	verifyToken(tokenType::TOKEN_SEMICOLON, "Expected ';' after client_max_body_size value");
}

void	Parser::parseInsideLocationBlock(routeConfig& rC)
{
	const Token& token = verifyToken(tokenType::TOKEN_WORD, "Expected directive in location");
	const std::string&	name = token.value;

	if (name == "methods")
		parseMethodsDirective(rC);
	else if (name == "root")
		parseRootDirective(rC);
	else if (name == "index")
		parseIndexDirective(rC);
	else if (name == "autoindex")
		parseAutoindexDirective(rC);
	else if (name == "upload_store")
		parseUploadStoreDirective(rC);
	else if (name == "client_max_body_size")
		parseBodySizeDirective(rC);
	else if (name == "cgi")
		parseCGIDirective(rC);
	else if (name == "return")
		parseReturnDirective(rC);
	else
		throwError("Unknown directive in location block: " + name);
}

static bool isValidLocationPath(const std::string& path)
{
	if (path.empty())
		return false;

	if (path[0] != '/')
		return false;
	
	for (size_t i = 1; i < path.size(); i++)
	{
		if (path[i] == '/' && path[i - 1] == '/')
			return false;
	}
	return true;
}

routeConfig	Parser::parseLocationBlock()
{
	routeConfig	rC;
	const Token& token = verifyToken(tokenType::TOKEN_WORD, "Expected location path after 'location' directive");
	rC.path = token.value;

	if (!isValidLocationPath(token.value))
		throwError("Invalid location path");

	verifyToken(tokenType::TOKEN_LBRACE, "Expected '{' after location path");

	while (!checkifEOF() && currentPosition().type != tokenType::TOKEN_RBRACE)
		parseInsideLocationBlock(rC);
	
	verifyToken(tokenType::TOKEN_RBRACE, "Expected '}' to close location block");
	return rC;
}

// Parsing of server block

void	Parser::isValidPort(int port)
{
	if (port < 1 || port > 65535)
		throwError("Port must be in between 1 and 65535");
}

static bool	isNumber(const std::string& nb)
{
	return (!nb.empty() && std::all_of(nb.begin(), nb.end(), ::isdigit));
}

void	Parser::isValidIP(const std::string& ip)
{
	std::stringstream	ss(ip);
	std::string			segment;
	int	count = 0;

	while (std::getline(ss, segment, '.'))
	{
		if (!isNumber(segment))
			throwError("Invalid IP value");

		int number = std::stoi(segment);
		if (number < 0 || number > 255)
			throwError("Invalid IP range");
		count++;
	}
	
	if (count != 4)
		throwError("Invalid IP address");
}

void	Parser::parseListenDirective(serverConfig& sC)
{
	const Token&	address = verifyToken(tokenType::TOKEN_WORD, "Expected address after 'listen' directive");

	std::string	value = address.value;

	std::string host = "127.0.0.1";
	int port;

	size_t	colon = value.find(':');
	if (colon == std::string::npos)
	{
		if (!isNumber(value))
			throwError("Invalid port values");
		
		port = std::stoi(value);
		isValidPort(port);
	}
	else
	{
		host = value.substr(0, colon);
		std::string	portString = value.substr(colon + 1);

		isValidIP(host);

		if (!isNumber(portString))
			throwError("Invalid port values");
		
		port = std::stoi(portString);
		isValidPort(port);
	}

	verifyToken(tokenType::TOKEN_SEMICOLON, "Expected ';' after listen value");

	if (sC.endpoint.ip == host && sC.endpoint.port == port)
		throwError("Duplicate listen values");	
	
	sC.endpoint.ip = host;
	sC.endpoint.port = port;

	sC.endpointSet = true;
}

static bool	isValidServerName(const std::string& name)
{
	if (name.empty())
		return false;

	if (name.front() == '.' || name.back() == '.')
		return false;
	
	for (size_t i = 0; i < name.size(); ++i)
	{
		char c = name[i];
		if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '.')
			return false;
	}
	return true;
}

void	Parser::parseServerNameDirective(serverConfig& sC)
{
	bool	parsedAtLeastOne = false;

	while (currentPosition().type == tokenType::TOKEN_WORD
		&& isValidServerName(currentPosition().value))
	{
		const std::string	value = currentPosition().value;

		if (std::find(sC.serverNames.begin(), sC.serverNames.end(), value) != sC.serverNames.end())
			throwError("Duplicate server name");

		sC.serverNames.push_back(value);
		moveForward();
		parsedAtLeastOne = true; 
	}
	if (!parsedAtLeastOne)
		throwError("There should be at least one server name");

	verifyToken(tokenType::TOKEN_SEMICOLON, "Expected ';' after server_name value");
}

void	Parser::parseBodySizeDirective(serverConfig& sC)
{
	if (sC.bodySizeSet)
		throwError("Duplicate client_max_body_size directive");
	
	sC.bodySizeSet = true;

	const Token&	bodySize = verifyToken(tokenType::TOKEN_WORD, "Expected number size after 'client_max_body_size' directive");
	
	try
	{
		sC.clientMaxBodySize = convertClientMaxBodySize(bodySize.value);
	}
	catch(const std::exception& e)
	{
		throwError("Invalid client_max_body_size value");
	}
	
	verifyToken(tokenType::TOKEN_SEMICOLON, "Expected ';' after client_max_body_size value");
}

void	Parser::parseErrorPageDirective(serverConfig& sC)
{
	std::vector<int>	errorCodes;

	while (currentPosition().type == tokenType::TOKEN_WORD)
	{
		const std::string& value = currentPosition().value;

		if (value.empty() || !std::all_of(value.begin(), value.end(), ::isdigit))
			break;
		int code = std::stoi(value);
		isValidStatusCode(code, StatusCodeMode::ErrorPage);
		errorCodes.push_back(code);

		moveForward();
	}

	if (errorCodes.empty())
		throwError("There should be at least one error code");

	const Token&	path = verifyToken(tokenType::TOKEN_WORD, "Expected path after error code");
	verifyToken(tokenType::TOKEN_SEMICOLON, "Expected ';' after error_page values");
	for (size_t i = 0; i < errorCodes.size(); i++)
	{
		if (sC.errorPages.find(errorCodes[i]) != sC.errorPages.end())
			throwError("Duplicate error_page directive");

		sC.errorPages[errorCodes[i]] = path.value;
	}	
}

void	Parser::parseInsideServerBlock(serverConfig& sC)
{
	const Token& token = verifyToken(tokenType::TOKEN_WORD, "Expected directive inside server block");
	const std::string& name = token.value;

	if (name == "location")
	{
		routeConfig route = parseLocationBlock();

		for (size_t i = 0; i < sC.routes.size(); i++)
		{
			if (sC.routes[i].path == route.path)
				throwError("Duplicate location path: " + route.path);
		}

		sC.routes.push_back(route);
		return;
	}
	else if (name == "listen")
		parseListenDirective(sC);
	else if (name == "server_name")
		parseServerNameDirective(sC);
	else if (name == "client_max_body_size")
		parseBodySizeDirective(sC);
	else if (name == "error_page")
		parseErrorPageDirective(sC);
	else
		throwError("Unknown directive inside server block: " + name);
}

serverConfig Parser::parseServerBlock()
{
    compareWord("server", "Expected 'server' block");
    verifyToken(tokenType::TOKEN_LBRACE, "Expected '{' after 'server'");

    serverConfig sC;

    while (!checkifEOF() && currentPosition().type != tokenType::TOKEN_RBRACE)
        parseInsideServerBlock(sC);

    verifyToken(tokenType::TOKEN_RBRACE, "Expected '}' to close server block");
	if (!sC.endpointSet)
		throwError("Server block requires at least one listen directive");

    return sC;
}

mainConfig	Parser::parse()
{
	mainConfig	mC;

	while (!checkifEOF())
	{
		if (currentPosition().type == tokenType::TOKEN_ERROR)
			throwError("Error with tokenizing");
		if (currentPosition().type == tokenType::TOKEN_EOF)
			break;
		mC.servers.push_back(parseServerBlock());
	}
	return mC;
}
