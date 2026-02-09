#ifndef TOKENIZER_HPP
# define TOKENIZER_HPP

# include <iostream>

enum tokenType
{
	TOKEN_WORD, // "lister", "server", "location", etc.
	TOKEN_LBRACE, // {
	TOKEN_RBRACE, // }
	TOKEN_SEMICOLON, // ;
	TOKEN_EOF,
	TOKEN_ERROR,
};

struct Token
{
	tokenType	type;
	std::string	value;
	int			line; // returns the line the token is in; useful for debugging/error handling
	int			column; // returns the column the token starts in; useful for debugging/error handling
};

class Tokenizer
{
	private:
		std::string _src;
		size_t		_index;
		int			_line;
		int			_col;

		char	curPos() const;
		char	cur_or(char fallback) const;
		bool	isEOF();
		void	moveForward();
		void	skipWhitespaceAndComments();

		Token	readIdentifier();
		Token	readQuotedWord();

	public:
		// Orthodox canonical form
		Tokenizer(std::string& input);
		Tokenizer(const Tokenizer& other);
		Tokenizer&	operator=(const Tokenizer& other);
		~Tokenizer();

		Token	nextToken();
		Token	peekToken();

		// Getters
		std::string	getSrc() const;
		size_t		getIndex() const;
		int			getLine() const;
		int			getCol() const;
};

#endif