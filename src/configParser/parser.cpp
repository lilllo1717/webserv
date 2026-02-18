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

const Token&	Parser::verifyToken(tokenType tok, const std::string& errorMessage)
{
	if (currentPosition().type == tokenType::TOKEN_ERROR)
		std::cerr << "Error with verifying token: " << currentPosition().value << std::endl;
	if (currentPosition().type != tok)
		std::cerr << errorMessage << std::endl;
	
	const Token&	token = currentPosition();
	moveForward();
	return token;
}

const Token&	Parser::checkTokenWord(const std::string& word, const std::string& errorMessage)
{
	const Token&	token = verifyToken(tokenType::TOKEN_WORD, errorMessage);
	if (token.value != word)
		std::cerr << "Token does not match relevant word" << std::endl;
	return token;
}

std::vector<std::string>	Parser::readArgumentsLine()
{
	std::vector<std::string>	arguments;

	while (!checkifEOF() && currentPosition().type != tokenType::TOKEN_SEMICOLON)
	{
		if (currentPosition().type == tokenType::TOKEN_LBRACE
			|| currentPosition().type == tokenType::TOKEN_RBRACE)
			std::cerr << "Unexpected brace in arguments line" << std::endl;
		const Token&	tok = verifyToken(tokenType::TOKEN_WORD, "Expected argument");
		arguments.push_back(tok.value);
	}
	verifyToken(tokenType::TOKEN_SEMICOLON, "Expected ';' after directive");
	return arguments;
}

size_t	Parser::parseClientMaxBodySize(std::string& nb)
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
	if (name == "root")
	{
		if (!arguments.empty())
			rC.rootDir = arguments[0];
		return;
	}
	if (name == "index")
	{
		if (!arguments.empty())
			rC.defaultFile = arguments[0];
		return;
	}
	if (name == "autoindex")
	{
		if (!arguments.empty())
			rC.autoindex = (arguments[0] == "on");
		return;
	}
	if (name == "upload_store")
	{
		rC.allow_upload = true;
		if (!arguments.empty())
			rC.uploadPath = arguments[0];
		return;
	}
	if (name == "cgi")
	{
		if (arguments.size() >= 2)
			rC.cgi[arguments[0]] = arguments[1];
		return;
	}
	if (name == "return")
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
void	Parser::parseInsideServerBlock(serverConfig& sC)
{
	const Token& token = verifyToken(tokenType::TOKEN_WORD, "Expected 'server' directive or 'location' directive");
	const std::string& name = token.value;

	std::vector<std::string>	arguments = readArgumentsLine();

	if (name == "listen")
	{
		sC.listen.push_back(arguments[0]);
		return;
	}
	if (name == "server_name")
	{
		sC.serverNames.push_back(arguments[0]);
		return;
	}
	if (name == "client_max_body_size")
	{
		if (!arguments.empty())
			sC.clientMaxBodySize = parseClientMaxBodySize(arguments[0]);
		return;
	}
	if (name == "error_page")
	{
		if (arguments.size() >= 2)
		{
			const std::string& path = arguments.back();
			for (size_t i = 0; i + 1 < arguments.size(); i++)
			{
				int errorCode = std::stoi(arguments[0]);
				sC.errorPages[errorCode] = path;
			}
		}
	}
	if (name == "location")
	{
		sC.routes.push_back(parseLocationBlock());
		return;
	}
}

mainConfig	Parser::parse()
{
	mainConfig	mC;

	while (!checkifEOF())
	{
		if (currentPosition().type == tokenType::TOKEN_ERROR)
			std::cerr << "Error with tokenizing" << std::endl;
		if (currentPosition().type == tokenType::TOKEN_EOF)
			break;
		mC.servers.push_back(parseServerBlock());
	}
	return mC;
}
