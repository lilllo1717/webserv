#ifndef TOKENIZER_HPP
# define TOKENIZER_HPP

# include <iostream>
# include <fstream>
# include <string>
# include <sstream>
# include <stdexcept>

enum class tokenType
{
	TOKEN_WORD, // "lister", "server", "location", etc.
	TOKEN_LBRACE, // {
	TOKEN_RBRACE, // }
	TOKEN_SEMICOLON, // ;
	TOKEN_EOF,
	TOKEN_ERROR, // invalid char or unterminated string
};

struct Token
{
	tokenType	type;
	std::string	value;
	int			line; // returns the line the token is in; useful for debugging/error handling
	int			column; // returns the column the token starts in; useful for debugging/error handling

	Token(tokenType t = tokenType::TOKEN_EOF, std::string v = "", int ln = 1, int col = 1)
		: type(t), value(v), line(ln), column(col) {}
};

class Tokenizer
{
	private:
		std::string _src;
		size_t		_idx;
		int			_line;
		int			_col;
		
		char	currentPosition() const;
		bool	isEOF();
		void	moveForward();
		void	skipWhitespaceAndComments();

		Token	readIdentifier();
		Token	readQuotedWord();

	public:
		Tokenizer(std::string& input): _src(input), _idx(0), _line(1), _col(1) {}

		Token	createToken();
};

std::string	readFile(const std::string& path);

#endif