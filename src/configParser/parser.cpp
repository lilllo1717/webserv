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
		convertedNb = convertedNb * 10 + (nb[j] - '0');
		j++;
	}
	if (i == nb.size())
		return convertedNb;
	if (i + 1 != nb.size())
		throw std::runtime_error("Invalid suffix format");
	
	char	suffix = nb[i];
	if (suffix == 'K')
		return convertedNb * 1024;
	if (suffix == 'M')
		return convertedNb * 1024 * 1024;
	if (suffix == 'G')
		return convertedNb * 1024 * 1024 * 1024;
	
	throw std::runtime_error("Unknown suffix specified");
}

// Parsing of location block (starting from the inside out)

static bool	isValidMethod(const std::string& method)
{
	return (method == "GET" || method == "POST" || method == "DELETE");
}
void	Parser::parseMethodsDirective(routeConfig& rC)
{
	if (currentPosition().type != tokenType::TOKEN_WORD)
		throwError("There should be at least one HTTP method after 'methods'");

	bool	parsedAtLeastOne = false;

	while (currentPosition().type == tokenType::TOKEN_WORD
		&& isValidMethod(currentPosition().value))
	{
		const std::string&	value = currentPosition().value;
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
	const Token&	root = verifyToken(tokenType::TOKEN_WORD, "Expected route path after 'root' directive");
	rC.rootDir = root.value;
	verifyToken(tokenType::TOKEN_SEMICOLON, "Expected ';' after root value");
}

void	Parser::parseIndexDirective(routeConfig& rC)
{
	const Token&	index = verifyToken(tokenType::TOKEN_WORD, "Expected index value after 'index' directive");
	rC.defaultFile = index.value;
	verifyToken(tokenType::TOKEN_SEMICOLON, "Expected ';' after index value");
}

void	Parser::parseAutoindexDirective(routeConfig& rC)
{
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

	rC.cgi[extension] = executable;
	verifyToken(tokenType::TOKEN_SEMICOLON, "Expected ';' after cgi values");
}

void	Parser::parseReturnDirective(routeConfig& rC)
{
	rC.isRedirect = true;

	if (currentPosition().type != tokenType::TOKEN_WORD || currentPosition().value.empty()
		|| !std::all_of(currentPosition().value.begin(), currentPosition().value.end(), ::isdigit))
	{
		throwError("There should be a return code after 'return' directive");
	}

	const std::string& value = currentPosition().value;
	int code = std::stoi(value);
	rC.redirectCode = code;
	moveForward();

	const Token&	redirectPath = verifyToken(tokenType::TOKEN_WORD, "Expected redirect target after error code");
	rC.redirectTarget = redirectPath.value;
	verifyToken(tokenType::TOKEN_SEMICOLON, "Expected ';' after return value");
}

void	Parser::parseInsideLocationBlock(routeConfig& rC)
{
	const Token& token = verifyToken(tokenType::TOKEN_WORD, "Expected directive in location");
	const std::string&	name = token.value;

	if (name == "methods")
	{
		return parseMethodsDirective(rC);
	}
	else if (name == "root")
	{
		return parseRootDirective(rC);
	}
	else if (name == "index")
	{
		return parseIndexDirective(rC);
	}
	else if (name == "autoindex")
	{
		 return parseAutoindexDirective(rC);
	}
	else if (name == "upload_store")
	{
		return parseUploadStoreDirective(rC);
	}
	else if (name == "cgi")
	{
		return parseCGIDirective(rC);
	}
	else if (name == "return")
	{
		return parseReturnDirective(rC);
	}
	else
	{
		throwError("Unknown directive in location block: " + name);
	}
}

routeConfig	Parser::parseLocationBlock()
{
	routeConfig	rC;
	const Token& token = verifyToken(tokenType::TOKEN_WORD, "Expected location path after 'location' directive");
	rC.path = token.value;

	verifyToken(tokenType::TOKEN_LBRACE, "Expected '{' after location path");

	while (!checkifEOF() && currentPosition().type != tokenType::TOKEN_RBRACE)
		parseInsideLocationBlock(rC);
	
	verifyToken(tokenType::TOKEN_RBRACE, "Expected '}' to close location block");
	return rC;
}

// Parsing of server block

void	Parser::parseListenDirective(serverConfig& sC)
{
	const Token&	address = verifyToken(tokenType::TOKEN_WORD, "Expected address after 'listen' directive");
	sC.listen.push_back(address.value);
	verifyToken(tokenType::TOKEN_SEMICOLON, "Expected ';' after listen value");
}

void	Parser::parseServerNameDirective(serverConfig& sC)
{
	bool	parsedAtLeastOne = false;

	while (currentPosition().type == tokenType::TOKEN_WORD)
	{
		sC.serverNames.push_back(currentPosition().value);
		moveForward();
		parsedAtLeastOne = true;
	}
	if (!parsedAtLeastOne)
		throwError("There should be at least one server name");
	verifyToken(tokenType::TOKEN_SEMICOLON, "Expected ';' after server_name value");
}

void	Parser::parseBodySizeDirective(serverConfig& sC)
{
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
		errorCodes.push_back(code);

		moveForward();
	}

	if (errorCodes.empty())
		throwError("There should be at least one error code");

	const Token&	path = verifyToken(tokenType::TOKEN_WORD, "Expected path after error code");
	verifyToken(tokenType::TOKEN_SEMICOLON, "Expected ';' after error_page values");
	for (size_t i = 0; i < errorCodes.size(); i++)
		sC.errorPages[errorCodes[i]] = path.value;
}

void	Parser::parseInsideServerBlock(serverConfig& sC)
{
	const Token& token = verifyToken(tokenType::TOKEN_WORD, "Expected directive inside server block");
	const std::string& name = token.value;

	if (name == "location")
	{
		sC.routes.push_back(parseLocationBlock());
		return;
	}
	else if (name == "listen")
	{
		parseListenDirective(sC);
	}
	else if (name == "server_name")
	{
		parseServerNameDirective(sC);
	}
	else if (name == "client_max_body_size")
	{
		parseBodySizeDirective(sC);
	}
	else if (name == "error_page")
	{
		parseErrorPageDirective(sC);
	}
	else
	{
		throwError("Unknown directive inside server block: " + name);
	}
}

serverConfig Parser::parseServerBlock()
{
    compareWord("server", "Expected 'server' block");
    verifyToken(tokenType::TOKEN_LBRACE, "Expected '{' after 'server'");

    serverConfig sC;

    while (!checkifEOF() && currentPosition().type != tokenType::TOKEN_RBRACE)
        parseInsideServerBlock(sC);

    verifyToken(tokenType::TOKEN_RBRACE, "Expected '}' to close server block");
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

static const char*	printTokenType(tokenType t)
{
	switch (t)
	{
		case tokenType::TOKEN_WORD:
			return "WORD";
		case tokenType::TOKEN_LBRACE:
			return "LEFT BRACE";
		case tokenType::TOKEN_RBRACE:
			return "RIGHT BRACE";
		case tokenType::TOKEN_SEMICOLON:
			return "SEMICOLON";
		case tokenType::TOKEN_EOF:
			return "EOF";
		case tokenType::TOKEN_ERROR:
			return "ERROR";
	}
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: ./webserv <config_file>\n";
        return 1;
    }

    try
    {
        std::string configText = readFile(argv[1]);

        Tokenizer tokenizer(configText);

        // test token output
		std::vector<Token>	tokens;
        Token token;
        do
		{
            token = tokenizer.createToken();
			tokens.push_back(token);
			std::cout << "[" << token.line << ":" << token.column << "] "
                  << printTokenType(token.type)
                  << " -> \"" << token.value << "\""
                  << std::endl;
        } while (token.type != tokenType::TOKEN_EOF);

		Parser parser(tokens);
		mainConfig	mC = parser.parse();

		for (size_t i = 0; i < mC.servers.size(); i++)
		{
			const serverConfig& sC = mC.servers[i];
			std::cout << std::endl;
			std::cout << "[Server " << i << "]" << std::endl;
            std::cout << "  listen entries: " << sC.listen.size() << std::endl;
            std::cout << "  server_names:   " << sC.serverNames.size() << std::endl;
            std::cout << "  routes:         " << sC.routes.size() << std::endl;
		}

    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
