#include "tokenizer.hpp"

std::string	readFile(const std::string& path)
{
    std::ifstream inputFile(path.c_str());
    if (!inputFile)
        throw std::runtime_error("Error opening input file");

    std::ostringstream buffer;
	buffer << inputFile.rdbuf();

    return buffer.str();
}

Tokenizer::Tokenizer(std::string& input): _src(input), _idx(0), _line(1), _col(1) {}

char	Tokenizer::currentPosition() const
{
	if (_idx >= _src.size())
		return '\0';
	return (_src[_idx]);
}

bool	Tokenizer::isEOF()
{
	return _idx >= _src.size();
}

void	Tokenizer::moveForward()
{
	if (isEOF())
		return ;
	
	if (_src[_idx] == '\n')
	{
		_line++;
		_col = 1;
	}
	else
		_col++;
	_idx++;
}

void	Tokenizer::skipWhitespaceAndComments()
{
	while (!isEOF())
	{
		while (std::isspace(static_cast<unsigned char>(currentPosition())))
			moveForward();
		if (currentPosition() == '#')
		{
			while(!isEOF() && currentPosition() != '\n')
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
	while (!isEOF() && isIdentifierChar(currentPosition()))
	{
		out.push_back(currentPosition());
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

	while (!isEOF() && currentPosition() != '"')
	{
		if (currentPosition() == '\\')
		{
			moveForward();
			if (isEOF())
				break ;
			
			char e = currentPosition();
			// out.push_back('\t');
			// break ;
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
					out.push_back(e);
					break ;
			}
			moveForward();
		}
		else
		{
			out.push_back(currentPosition());
			moveForward();
		}
	}

	if (currentPosition() != '"' || isEOF())
		return (Token(tokenType::TOKEN_ERROR, "unterminated string", startLine, startCol)); // string with unclosed quote

	moveForward(); // consume closing quote
	return (Token(tokenType::TOKEN_WORD, out, startLine, startCol));
}

Token	Tokenizer::createToken()
{
	skipWhitespaceAndComments();

	const int	startLine = _line;
	const int	startCol = _col;

	if (isEOF())
		return (Token(tokenType::TOKEN_EOF, "", startLine, startCol));

	char c = currentPosition();

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

	if (isIdentifierChar(c))
		return readIdentifier();
	
	std::string	errorMessage = "Unexpected character: '";
	errorMessage.push_back(c);
	errorMessage.push_back('\'');
	moveForward();
	return Token(tokenType::TOKEN_ERROR, errorMessage, startLine, startCol);
}
