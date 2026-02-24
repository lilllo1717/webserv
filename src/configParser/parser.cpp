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

std::vector<std::string>	Parser::readArgumentsLine()
{
	std::vector<std::string>	arguments;

	while (!checkifEOF() && currentPosition().type != tokenType::TOKEN_SEMICOLON)
	{
		if (currentPosition().type == tokenType::TOKEN_LBRACE
			|| currentPosition().type == tokenType::TOKEN_RBRACE)
			throwError("Unexpected brace in arguments line");
		const Token&	tok = verifyToken(tokenType::TOKEN_WORD, "Expected argument");
		arguments.push_back(tok.value);
	}
	verifyToken(tokenType::TOKEN_SEMICOLON, "Expected ';' after directive");
	return arguments;
}

size_t	Parser::parseClientMaxBodySize(const std::string& nb)
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
	while (currentPosition().type == tokenType::TOKEN_WORD)
	{
		const std::string&	value = currentPosition().value;
		if (!isValidMethod(value))
			throwError("Invalid methods: only 'GET', 'POST', 'DELETE' are allowed");
		rC.httpMethods.push_back(value);
		moveForward();
	}
	// const Token&	methods
	// rC.httpMethods.push_back(methods.value);
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
	else if (autoindex. value == "off")
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
	return (file == ".php" || file == ".py" || file == ".pl" || file == ".c" || file == ".cpp");
}

void	Parser::parseCGIDirective(routeConfig& rC)
{
	std::vector<std::string>	fileExtensions;

	while (currentPosition().type == tokenType::TOKEN_WORD)
	{
		const std::string&	value = currentPosition().value;

		if (!(!value.empty() && std::all_of(value.begin(), value.end(), isValidFileExtenstion(value))))
			break;
		fileExtensions.push_back(value);

		moveForward();
	}
	const Token&	filePath = verifyToken(tokenType::TOKEN_WORD, "Expected file path after file extenstion");
	verifyToken(tokenType::TOKEN_SEMICOLON, "Expected ';' after cgi values");
	for (size_t i = 0; i < fileExtensions.size(); i++)
		rC.cgi[fileExtensions[i]] = filePath.value;
}

void	Parser::parseInsideLocationBlock(routeConfig& rC)
{
	const Token& token = verifyToken(tokenType::TOKEN_WORD, "Expected directive in location");
	const std::string&	name = token.value;

	std::vector<std::string> arguments = readArgumentsLine();

	if (name == "methods")
	{
		if (!arguments.empty())
			rC.httpMethods = arguments;
		return;
	}
	else if (name == "root")
	{
		if (!arguments.empty())
			rC.rootDir = arguments[0];
		return;
	}
	else if (name == "index")
	{
		if (!arguments.empty())
			rC.defaultFile = arguments[0];
		return;
	}
	else if (name == "autoindex")
	{
		if (!arguments.empty())
			rC.autoindex = (arguments[0] == "on");
		return;
	}
	else if (name == "upload_store")
	{
		rC.allow_upload = true;
		if (!arguments.empty())
			rC.uploadPath = arguments[0];
		return;
	}
	else if (name == "cgi")
	{
		if (arguments.size() >= 2)
			rC.cgi[arguments[0]] = arguments[1];
		return;
	}
	else if (name == "return")
	{
		if (arguments.size() >= 2)
		{
			rC.isRedirect = true;
			rC.redirectCode = std::stoi(arguments[0]);
			rC.redirectTarget = arguments[1];
		}
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
	const Token&	name = verifyToken(tokenType::TOKEN_WORD, "Expected server name after 'server_name' directive");
	sC.serverNames.push_back(name.value);
	verifyToken(tokenType::TOKEN_SEMICOLON, "Expected ';' after server_name value");
}

void	Parser::parseBodySizeDirective(serverConfig& sC)
{
	const Token&	bodySize = verifyToken(tokenType::TOKEN_WORD, "Expected number size after 'client_max_body_size' directive");
	sC.clientMaxBodySize = parseClientMaxBodySize(bodySize.value);
	verifyToken(tokenType::TOKEN_SEMICOLON, "Expected ';' after client_max_body_size value");
}

void	Parser::parseErrorPageDirective(serverConfig& sC)
{
	std::vector<int>	errorCodes;

	while (currentPosition().type == tokenType::TOKEN_WORD)
	{
		const std::string& value = currentPosition().value;

		if (!(!value.empty() && std::all_of(value.begin(), value.end(), ::isdigit)))
			break;
		int code = std::stoi(value);
		errorCodes.push_back(code);

		moveForward();
	}
	const Token&	path = verifyToken(tokenType::TOKEN_WORD, "Expected path after error code");
	verifyToken(tokenType::TOKEN_SEMICOLON, "Expected ';' after error_page values");
	for (size_t i = 0; i < errorCodes.size(); i++)
		sC.errorPages[errorCodes[i]] = path.value;
}

void	Parser::parseInsideServerBlock(serverConfig& sC)
{
	const Token& token = verifyToken(tokenType::TOKEN_WORD, "Expected 'server' directive");
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
		throwError("Unknown directive specified");
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
