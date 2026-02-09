#include "tokenizer.hpp"

Tokenizer::Tokenizer(std::string& input): _src(input), _index(0), _line(1), _col(1) {}

char	Tokenizer::curPos() const
{
	if (_index >= _src.size())
		return '\0';
	return (_src[_index]);
}

char	Tokenizer::cur_or(char fallback) const
{
	char c = curPos();
	return (c == '\0') ? fallback : c;
}

bool	Tokenizer::isEOF()
{
	return _index >= _src.size();
}

void	Tokenizer::moveForward()
{
	if (isEOF())
		return ;
	
	if (_src[_index] == '\n')
	{
		_line++;
		_col = 1;
	}
	else
		_col++;
	_index++;
}

void	Tokenizer::skipWhitespaceAndComments()
{
	while (!isEOF())
	{
		while (std::isspace(curPos()))
			moveForward();
		if (curPos() == '#')
		{
			while(!isEOF() && curPos() != '\n')
				moveForward();
			continue ;
		}
		break ;
	}
}

static bool	isIdentifierChar(char c)
{
	unsigned char uc = static_cast<unsigned char>(c);

	return std::isalnum(uc)
		|| c == '_'
		|| c == '-'
		|| c == '/'
		|| c == '.'
		|| c == ':'
		|| c == '@'
		|| c == '+';
}

Token	Tokenizer::readIdentifier()
{

}

Token	Tokenizer::readQuotedWord()
{

}

Token	Tokenizer::nextToken()
{

}

Token	Tokenizer::peekToken()
{
	
}


