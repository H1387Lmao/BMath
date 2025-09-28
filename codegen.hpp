#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <memory>
#include "node.hpp"

// Simple NASM-style assembly code generator for x86 (32-bit and 64-bit)
// - Emits a C-linkable `main` function so it works on Linux and Windows when linked via a C/C++ toolchain
// - Integer-only arithmetic
// - Supports variables via global .bss symbols (assignment and usage in expressions)
// - Binary ops: +, -, *, /, %
// - Results of the last statement returned as int from main

enum class TargetArch { X86_32, X86_64 };
enum class TargetOS { Linux, Windows };

struct CodegenOptions {
    TargetArch arch{TargetArch::X86_64};
    TargetOS os{TargetOS::Linux};
};

namespace codegen_detail {
    inline bool is_number_token(const Token& tk) {
        return tk.t_type == NUMBERLITERAL;
    }

    inline bool is_identifier_token(const Token& tk) {
        return tk.t_type == IDENTIFIER;
    }

    // Collect variable identifiers from the AST
    inline void collect_vars(const AstNode& node, std::unordered_set<std::string>& vars) {
        if (node.n_type == ASSIGN) {
            // args[0] should be identifier token
            for (const auto& a : node.args) {
                if (a.tk && a.tk->t_type == IDENTIFIER) {
                    vars.insert(a.tk->value);
                    break;
                }
            }
            // rhs may introduce identifiers too
            for (const auto& a : node.args) {
                if (a.node) collect_vars(*a.node, vars);
            }
            return;
        }
        if (node.n_type == BINOP) {
            for (const auto& a : node.args) {
                if (a.node) collect_vars(*a.node, vars);
                if (a.tk && a.tk->t_type == IDENTIFIER) vars.insert(a.tk->value);
            }
            return;
        }
        if (node.n_type == LITERAL) {
            for (const auto& a : node.args) {
                if (a.tk && a.tk->t_type == IDENTIFIER) vars.insert(a.tk->value);
            }
            return;
        }
        if (node.n_type == PROG) {
            for (const auto& a : node.args) {
                if (a.node) collect_vars(*a.node, vars);
            }
            return;
        }
    }

    struct Emitter {
        const CodegenOptions& opts;
        std::ostringstream text;
        std::ostringstream bss;
        std::unordered_map<std::string, bool> declared; // var -> declared in .bss

        inline bool is64() const { return opts.arch == TargetArch::X86_64; }

        inline void declare_var(const std::string& name) {
            if (declared.count(name)) return;
            declared[name] = true;
            if (is64()) {
                bss << name << ": resq 1\n"; // 8 bytes
            } else {
                bss << name << ": resd 1\n"; // 4 bytes
            }
        }

        // Emit code that leaves result in eax/rax
        void emit_expr(const AstNode& node) {
            switch (node.n_type) {
                case LITERAL: {
                    // expect a single token argument
                    const Token* tk = nullptr;
                    for (const auto& a : node.args) if (a.tk) { tk = &*a.tk; break; }
                    if (!tk) { text << (is64()?"xor rax, rax\n":"xor eax, eax\n"); return; }
                    if (tk->t_type == NUMBERLITERAL) {
                        if (is64()) text << "mov rax, " << tk->value << "\n";
                        else text << "mov eax, " << tk->value << "\n";
                    } else if (tk->t_type == IDENTIFIER) {
                        if (is64()) {
                            text << "mov rax, [" << tk->value << "]\n";
                        } else {
                            text << "mov eax, [" << tk->value << "]\n";
                        }
                    } else {
                        // unhandled literal kinds -> 0
                        text << (is64()?"xor rax, rax\n":"xor eax, eax\n");
                    }
                    break;
                }
                case BINOP: {
                    // Expect args: node(left), token(op), node(right)
                    const AstNode* left = nullptr;
                    const AstNode* right = nullptr;
                    const Token* op = nullptr;
                    if (node.args.size() >= 3) {
                        if (node.args[0].node) left = node.args[0].node.get();
                        if (node.args[1].tk) op = &*node.args[1].tk;
                        if (node.args[2].node) right = node.args[2].node.get();
                    }
                    if (!left || !right || !op) {
                        text << (is64()?"xor rax, rax\n":"xor eax, eax\n");
                        return;
                    }
                    // Evaluate left, save, then evaluate right
                    emit_expr(*left);
                    if (is64()) text << "push rax\n"; else text << "push eax\n";
                    emit_expr(*right);
                    if (is64()) text << "pop rbx\n"; else text << "pop ebx\n";

                    const std::string& o = op->value;
                    if (o == "+") {
                        if (is64()) text << "add rax, rbx\n"; else text << "add eax, ebx\n";
                    } else if (o == "-") {
                        if (is64()) {
                            text << "mov rcx, rax\n";
                            text << "mov rax, rbx\n";
                            text << "sub rax, rcx\n";
                        } else {
                            text << "mov ecx, eax\n";
                            text << "mov eax, ebx\n";
                            text << "sub eax, ecx\n";
                        }
                    } else if (o == "*") {
                        if (is64()) text << "imul rax, rbx\n"; else text << "imul eax, ebx\n";
                    } else if (o == "/" || o == "%") {
                        // signed division: rdx:rax / rbx -> rax rem rdx
                        if (is64()) {
                            text << "mov rcx, rax\n";       // rcx = right
                            text << "mov rax, rbx\n";       // rax = left
                            text << "cqo\n";                // sign-extend into rdx
                            text << "idiv rcx\n";           // rax = quot, rdx = rem
                            if (o == "%") text << "mov rax, rdx\n";
                        } else {
                            text << "mov ecx, eax\n";       // ecx = right
                            text << "mov eax, ebx\n";       // eax = left
                            text << "cdq\n";                // sign-extend into edx
                            text << "idiv ecx\n";           // eax = quot, edx = rem
                            if (o == "%") text << "mov eax, edx\n";
                        }
                    } else {
                        // unknown op -> 0
                        text << (is64()?"xor rax, rax\n":"xor eax, eax\n");
                    }
                    break;
                }
                case ASSIGN: {
                    // args[0] token(IDENT), args[1] node(expr)
                    const Token* id = nullptr;
                    const AstNode* rhs = nullptr;
                    if (node.args.size() >= 2) {
                        if (node.args[0].tk) id = &*node.args[0].tk;
                        if (node.args[1].node) rhs = node.args[1].node.get();
                    }
                    if (!id || !rhs) {
                        text << (is64()?"xor rax, rax\n":"xor eax, eax\n");
                        return;
                    }
                    declare_var(id->value);
                    emit_expr(*rhs); // result in rax/eax
                    if (is64()) text << "mov [" << id->value << "], rax\n";
                    else text << "mov [" << id->value << "], eax\n";
                    // Leave result of assignment in rax/eax
                    break;
                }
                case PROG: {
                    // Evaluate each stmt; the last one's value is returned by main
                    for (const auto& a : node.args) {
                        if (a.node) emit_expr(*a.node);
                    }
                    break;
                }
                default: {
                    text << (is64()?"xor rax, rax\n":"xor eax, eax\n");
                }
            }
        }
    };
}

inline std::string generate_asm(const AstNode& ast, const CodegenOptions& options) {
    using namespace codegen_detail;

    // First pass: collect variables for .bss
    std::unordered_set<std::string> vars;
    collect_vars(ast, vars);

    Emitter E{options};
    for (const auto& v : vars) E.declare_var(v);

    std::ostringstream out;

    // Sections and globals
    out << "section .text\n";
    if (options.arch == TargetArch::X86_64) out << "default rel\n";
    out << "global main\n";
    out << "main:\n";
    if (options.arch == TargetArch::X86_64) {
        out << "  push rbp\n  mov rbp, rsp\n";
    } else {
        out << "  push ebp\n  mov ebp, esp\n";
    }

// Emit program; result in rax/eax
    E.emit_expr(ast);
    out << E.text.str();

    // Move result to 32-bit return (if 64-bit, C ABI returns in eax lower 32 for int)
    if (options.arch == TargetArch::X86_64) {
        out << "  ; function epilogue\n  mov eax, eax\n  mov rsp, rbp\n  pop rbp\n  ret\n";
    } else {
        out << "  ; function epilogue\n  mov eax, eax\n  mov esp, ebp\n  pop ebp\n  ret\n";
    }

    if (!E.bss.str().empty()) {
        out << "section .bss\n";
        out << E.bss.str();
    }

    return out.str();
}
