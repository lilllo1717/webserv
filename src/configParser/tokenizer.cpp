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
	const int	startLine = _line;
	const int	startCol = _col;

	std::string out;
	while (!isEOF() && isIdentifierChar(curPos()))
	{
		out.push_back(curPos());
		moveForward();
	}
	return Token(tokenType::TOKEN_WORD, out, startLine, startCol);
}

Token	Tokenizer::readQuotedWord()
{
	const int	startLine = _line;
	const int	startCol = _col;

	moveForward();
	std::string out;

	while (!isEOF() && curPos() != '"')
	{
		if (curPos() == '\\')
		{
			moveForward();
			if (isEOF())
				break ;
			
			char e = curPos();
			switch (e)
			{
				case '"':
					out.push_back('"');
					break ;
				case '\\':
					out.push_back('\\');
					break ;
				case '\n':
					out.push_back('\n');
					break ;
				case '\t':
					out.push_back('\t');
					break ;
				default:
					out.push_back('\e');
					break ;
			}
		}
		else
		{
			out.push_back(curPos());
			moveForward();
		}
	}

	if (curPos() != '"')
		return (Token(tokenType::TOKEN_ERROR, "unterminated string", startLine, startCol)); // string with unclosed quote

	moveForward(); // consume closing quote
	return (Token(tokenType::TOKEN_WORD, out, startLine, startCol));
}

Token	Tokenizer::nextToken()
{
	const int	startLine = _line;
	const int	startCol = _col;

	if (isEOF())
		return (Token(tokenType::TOKEN_EOF, "", startLine, startCol));

	char c = curPos();

	if (c == '{')
	{
		moveForward();
		return Token(tokenType::TOKEN_LBRACE, "{", startLine, startCol);
	}

	if (c == '}')
	{
		moveForward();
		return Token(tokenType::TOKEN_RBRACE, "}", startLine, startCol);
	}

	if (c == ';')
	{
		moveForward();
		return Token(tokenType::TOKEN_SEMICOLON, ";", startLine, startCol);
	}

	if (c == '"')
		return (readQuotedWord());
	
	std::string	errorMsg = "Unexpected character: '";
	errorMsg.push_back(c);
	errorMsg.push_back('\'');
	moveForward();
	return Token(tokenType::TOKEN_ERROR, errorMsg, startLine, startCol);
}

Token	Tokenizer::peekToken()
{
	// Save current state before peeking
	std::size_t	savedPos = _index;
	int	savedLine = _line;
	int savedCol = _col;

	// Peek at the next token
	Token t = nextToken();

	// After peeking, go back to previous state
	_index = savedPos;
	_line = savedLine;
	_col = savedCol;

	return (t);
}
