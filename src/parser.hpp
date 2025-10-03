#pragma once
#include <vector>
#include "token.hpp"
#include "node.hpp"

using TokenList = std::vector<Token>;

// Parse a program from tokens into an AST
AstNode parse_prog(TokenList& tks);
