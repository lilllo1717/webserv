#include "parser.hpp"

Parser::Parser() {};
Parser::~Parser() {};

// Traversing through the vector of tokens and doing some verification

Token&	Parser::currentPosition()
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

Token&	Parser::moveForward()
{
	if (!checkifEOF())
		_position++;
	return currentPosition();
}

Token&	Parser::verifyToken(tokenType tok, std::string& errorMessage)
{
	if (currentPosition().type == tokenType::TOKEN_ERROR)
		std::cerr << "Error with verifying token: " << currentPosition().value << std::endl;
	if (currentPosition().type != tok)
		std::cerr << errorMessage << std::endl;
	
	Token&	token = currentPosition();
	moveForward();
	return token;
}

Token&	Parser::checkTokenWord(std::string& word, std::string& errorMessage)
{
	Token&	token = verifyToken(tokenType::TOKEN_WORD, errorMessage);
	if (token.value != word)
		std::cerr << "Token does not match relevant word" << std::endl;
	return token;
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

mainConfig	Parser::parse()
{

}