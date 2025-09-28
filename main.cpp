#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "lexer.hpp"
#include "parser.hpp"
#include "node.hpp"
#include "codegen.hpp"

// Helper display functions moved from parser.cpp
static std::string token_type_to_string(token_t type){
    switch (type){
        case IDENTIFIER: return "ID";
        case NUMBERLITERAL: return "NUML";
        case CHARLITERAL: return "CHARL";
        case STRINGLITERAL: return "STRL";
        case SYMBOLIC: return "SYMB";
        case _EOF: return "EOF";
        default: return "UNKNOWN";
    }
}

static std::string node_type_to_string(NodeType type){
    switch (type){
        case PROG: return "PROG";
        case BINOP: return "BINOP";
        case LITERAL: return "LITERAL";
        case ASSIGN: return "ASSIGN";
        default: return "UNKNOWN";
    }
}

static void display_hierarch(const AstNode& node, int indent){
    std::string pad(indent, ' ');
    std::cout << pad << "Node: " << node_type_to_string(node.n_type) << "\n";
    for (const auto& arg : node.args){
        if (arg.tk){
            const auto& tok = arg.tk.value();
            std::cout << pad << "  Token(" << token_type_to_string(tok.t_type) << "): " << tok.value << "\n";
        }
        if (arg.node){
            display_hierarch(*arg.node, indent+2);
        }
    }
}

static std::string slurp_stdin_line(){
    std::string input;
    if (!std::getline(std::cin, input)) return std::string();
#ifdef _WIN32
    if (!input.empty() && input.back() == '\r') input.pop_back();
#endif
    return input;
}

static bool read_file(const char* path, std::string& out){
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return false;
    std::ostringstream oss;
    oss << ifs.rdbuf();
    out = oss.str();
    return true;
}

static void detect_defaults(CodegenOptions& opts){
#ifdef _WIN32
    opts.os = TargetOS::Windows;
#else
    opts.os = TargetOS::Linux;
#endif
#if defined(_M_X64) || defined(__x86_64__) || defined(__amd64__)
    opts.arch = TargetArch::X86_64;
#else
    opts.arch = TargetArch::X86_32;
#endif
}

int main(int argc, char** argv){
    // CLI: bmath [input-file] [-o output-asm]
    // If -o is provided, generate NASM assembly to file. Otherwise, print AST.
    std::string input;
    std::string input_path;
    std::string out_path; // assembly output, optional

    // Parse args (very simple)
    for (int i = 1; i < argc; ++i){
        std::string arg = argv[i];
        if (arg == "-o" && i + 1 < argc){
            out_path = argv[++i];
        } else if (arg == "-h" || arg == "--help"){
            std::cout << "Usage: bmath [input-file] [-o output.asm]\n";
            return 0;
        } else if (!arg.empty() && arg[0] == '-'){
            std::cerr << "Unknown option: " << arg << "\n";
            return 1;
        } else {
            input_path = arg;
        }
    }

    if (!input_path.empty()){
        if (!read_file(input_path.c_str(), input)){
            std::cerr << "Error: failed to open file: " << input_path << "\n";
            return 1;
        }
    } else {
        input = slurp_stdin_line();
        if (input.empty()) return 0; // no input
    }

    // Tokenize and parse
    TokenList tokens = tokenize(input.c_str());
    AstNode ast = parse_prog(tokens);

    if (!out_path.empty()){
        // Codegen to assembly file
        CodegenOptions opts; detect_defaults(opts);
        std::string asmText = generate_asm(ast, opts);
        std::ofstream ofs(out_path, std::ios::binary);
        if (!ofs){
            std::cerr << "Error: failed to open output file: " << out_path << "\n";
            return 1;
        }
        ofs << asmText;
        ofs.close();
        std::cout << "Wrote assembly to " << out_path << "\n";
    } else {
        CodegenOptions opts; detect_defaults(opts);
        std::string asmText = generate_asm(ast, opts);
        
        display_hierarch(ast, 0);
        std::cout << asmText << "\n";
    }

    return 0;
}
