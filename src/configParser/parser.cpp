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


mainConfig	Parser::parse()
{

}