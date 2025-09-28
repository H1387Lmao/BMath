#pragma once
#include <iostream>

typedef enum token_t {
	IDENTIFIER,
	NUMBERLITERAL,
	CHARLITERAL,
	STRINGLITERAL,
	SYMBOLIC,
	_EOF
} token_t;

struct Token{
	token_t t_type;
	std::string value;
	Token(token_t type, std::string val): t_type(type), value(std::move(val)) {}
};
