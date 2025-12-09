#ifndef TOKENIZER_HPP
# define TOKENIZER_HPP

# include <iostream>

enum tokenType
{
	TOKEN_SERVER,
	TOKEN_WORD,
	TOKEN_LBRACE,
	TOKEN_RBRACE,
	TOKEN_SEMICOLON,
	TOKEN_EOF,
};

struct Token
{
	tokenType	type;
	std::string	value;
	int			line;
	int			column;
};

class Tokenizer
{
	private:
		std::string _src;
		size_t		_index;
		int			_line;
		int			_col;

	public:
		Tokenizer(std::string& src);
		Tokenizer(const Tokenizer& other);
		Tokenizer&	operator=(const Tokenizer& other);
		~Tokenizer();

};

#endif