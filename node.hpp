#pragma once
#include <vector>
#include <optional>
#include <memory>
#include "token.hpp"

enum NodeType{
	PROG,
	BINOP,
	LITERAL,
	ASSIGN
};

struct AstNode;

struct NodeArg{
	std::optional<Token> tk;
	std::shared_ptr<AstNode> node;
	NodeArg(std::optional<Token> t, std::shared_ptr<AstNode> a) : tk(std::move(t)), node(std::move(a)) {}
};

struct AstNode{
	NodeType n_type;
	std::vector<NodeArg> args;

	AstNode(NodeType n, std::vector<NodeArg> a): n_type(n), args(a) {}
};

NodeArg new_arg_token(Token t);
NodeArg new_arg_node(std::shared_ptr<AstNode> a);
std::vector<NodeArg> new_args(NodeArg a);
