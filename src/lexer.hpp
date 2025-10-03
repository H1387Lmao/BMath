#include "token.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <cctype>

bool is_alpha(char c){
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool is_num(char c){
	return (c >= '0' && c <= '9');
}

bool is_whitespace(char c){
	return (c == ' ' || c == '\n' || c == '\t');
}

std::vector<Token> tokenize(const char* src){
	std::vector<Token> tokens;
	const char* p = src;

while(*p != '\0'){
		if (is_whitespace(*p)){
			++p;
			continue;
		}
		if (is_alpha(*p)){
			std::string res(1, *p);
			++p;
			while (is_alpha(*p)){
				res += *p;
				++p;
			}
			tokens.emplace_back(IDENTIFIER, res);
		}else if (is_num(*p)){
			std::string res(1, *p);
			++p;
			while (is_num(*p)){
				res += *p;
				++p;
			}
			tokens.emplace_back(NUMBERLITERAL, res);
		}else if(*p == '"'){
			++p;
			std::string res(1, *p);
			++p;
			while (*p!= '"'){
				res += *p;
				++p;
			}
			tokens.emplace_back(STRINGLITERAL, res);
			++p;
		}else if(*p == '\''){
			++p;
			std::string res(1, *p);
			tokens.emplace_back(CHARLITERAL, res);
			++p;
			if (*p != '\''){
				std::cerr << "Expected single quote (') to finish character, but found (' " << *p << " ') instead\n";
			}else{
				++p;
			}
			
		}else{
			tokens.emplace_back(SYMBOLIC, std::string(1, *p));
			++p;
		}
	}
	// Append EOF once after tokenization
	tokens.emplace_back(_EOF, "EOF");
	// Debug print
	for (const auto& token : tokens){
		std::cout << "Token Type: " << token.t_type << ", Value: \"" << token.value << "\"\n";
	}
	return tokens;
}

std::vector<Token> lexer_main(){
	const char* str = "10+20";
	return tokenize(str);
}
