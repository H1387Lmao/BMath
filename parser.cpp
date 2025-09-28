#include "node.hpp"
#include "parser.hpp"
#include <iostream>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>

using NodeArgs = std::vector<NodeArg>;

template<typename T>
void pop_front(std::vector<T>& v){
	if (!v.empty()) {
		v.erase(v.begin());
	}
}

NodeArg new_arg_token(Token t){
	return NodeArg(std::move(t), nullptr);
}

NodeArg new_arg_node(std::shared_ptr<AstNode> a){
	return NodeArg(std::nullopt, std::move(a));
}

std::vector<NodeArg> new_args(NodeArg a){
	std::vector<NodeArg> args;
	args.emplace_back(std::move(a));
	return args;
}

// Helpers
static bool is_symbol(TokenList& tks, const char* s){
	return !tks.empty() && tks.front().t_type == SYMBOLIC && tks.front().value == s;
}

static bool at_eof(TokenList& tks){
	return tks.empty() || tks.front().t_type == _EOF;
}

static Token consume(TokenList& tks){
	Token tk = tks.front();
	pop_front(tks);
	return tk;
}

// Forward declarations
static std::shared_ptr<AstNode> parse_expr(TokenList& tks);

// value := NUMBER | IDENTIFIER | '(' expr ')'
static std::shared_ptr<AstNode> parse_value(TokenList& tks){
	if (at_eof(tks)){
		return std::make_shared<AstNode>(LITERAL, NodeArgs{});
	}
	if (is_symbol(tks, "(")){
		consume(tks); // '('
		auto inner = parse_expr(tks);
		if (is_symbol(tks, ")")) consume(tks); // ')'
		return inner;
	}
	if (tks.front().t_type == NUMBERLITERAL || tks.front().t_type == IDENTIFIER ||
								 tks.front().t_type == STRINGLITERAL || tks.front().t_type == CHARLITERAL){
		Token tk = consume(tks);
		return std::make_shared<AstNode>(LITERAL, new_args(new_arg_token(tk)));
	}
	// Fallback: treat as literal unknown token to avoid infinite loops
	Token tk = consume(tks);
	return std::make_shared<AstNode>(LITERAL, new_args(new_arg_token(tk)));
}

// term := value (('*' | '/' | '%') value)*
static std::shared_ptr<AstNode> parse_term(TokenList& tks){
	auto left = parse_value(tks);
	while (is_symbol(tks, "*") || is_symbol(tks, "/") || is_symbol(tks, "%")){
		Token op = consume(tks);
		auto right = parse_value(tks);
		NodeArgs args = new_args(new_arg_node(left));
		args.push_back(new_arg_token(op));
		args.push_back(new_arg_node(right));
		left = std::make_shared<AstNode>(BINOP, std::move(args));
	}
	return left;
}

// expr := term (("+" | "-") term)*
static std::shared_ptr<AstNode> parse_expr(TokenList& tks){
	auto left = parse_term(tks);
	while (is_symbol(tks, "+") || is_symbol(tks, "-")){
		Token op = consume(tks);
		auto right = parse_term(tks);
		NodeArgs args = new_args(new_arg_node(left));
		args.push_back(new_arg_token(op));
		args.push_back(new_arg_node(right));
		left = std::make_shared<AstNode>(BINOP, std::move(args));
	}
	return left;
}

std::shared_ptr<AstNode> parse_stmt(TokenList& tks){
	// assignment: IDENTIFIER '=' expr
	if (tks.size() >= 2 && tks[0].t_type == IDENTIFIER && tks[1].t_type == SYMBOLIC && tks[1].value == "="){
		Token id = consume(tks); // IDENT
		consume(tks); // '='
		auto rhs = parse_expr(tks);
		NodeArgs args = new_args(new_arg_token(id));
		args.push_back(new_arg_node(rhs));
		return std::make_shared<AstNode>(ASSIGN, std::move(args));
	}
	return parse_expr(tks);
}

AstNode parse_prog(TokenList& tks){
	NodeArgs args;
	while (!tks.empty() && tks.front().t_type != _EOF){
		args.push_back(new_arg_node(parse_stmt(tks)));
	}
	return AstNode(PROG, args);
}

