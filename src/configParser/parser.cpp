#include "parser.hpp"

Parser::Parser() {};

Parser::~Parser() {};

Token&	Parser::curPos()
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
	return curPos().type == tokenType::TOKEN_EOF;
}

Token&	Parser::moveForward()
{
	if (!checkifEOF())
		_position++;
	return curPos();
}


mainConfig	Parser::parse()
{

}