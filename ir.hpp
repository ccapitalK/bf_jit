#pragma once

#include <iostream>

#include "error.hpp"

enum class IROpCode {
    INS_ADD,    // Add to current cell
    INS_ZERO,   // Set current cell to zero
    INS_ADP,    // Add to data pointer
    INS_ACL,    // Unused (add multiple?)
    INS_IN,     // call mgetc()
    INS_OUT,    // call mputc()
    INS_LOOP,   // Start of loop
    INS_END,    // End of loop
    INS_INVALID // Not a valid instruction
};

struct Instruction {
    IROpCode code_{};
    int a_{};
    int b_{};
    int c_{};
    Instruction() : code_{IROpCode::INS_INVALID} {}
    Instruction(IROpCode code) : code_(code), a_(0), b_(0), c_(0) {}
    Instruction(IROpCode code, int a) : code_(code), a_(a), b_(0), c_(0) {}
    Instruction(IROpCode code, int a, int b) : code_(code), a_(a), b_(b), c_(0) {}
    Instruction(IROpCode code, int a, int b, int c) : code_(code), a_(a), b_(b), c_(c) {}
    bool isFoldable() {
        return code_ == IROpCode::INS_ADD || code_ == IROpCode::INS_ADP;
    }
    friend std::istream& operator>>(std::istream &is, Instruction &ins);
    friend std::ostream& operator<<(std::ostream &os, const Instruction &ins);
};

std::istream& operator>>(std::istream &is, Instruction &ins) {
    ins.code_ = IROpCode::INS_INVALID;
    char opc;
    is >> opc;
    if (!is.good()) return is;
    switch(opc) {
    case ',': ins.code_ = IROpCode::INS_IN; break;
    case '.': ins.code_ = IROpCode::INS_OUT; break;
    case '[': ins.code_ = IROpCode::INS_LOOP; break;
    case ']': ins.code_ = IROpCode::INS_END; break;
    case '+': ins.code_ = IROpCode::INS_ADD; ins.a_ = 1; break;
    case '-': ins.code_ = IROpCode::INS_ADD; ins.a_ = -1; break;
    case '>': ins.code_ = IROpCode::INS_ADP; ins.a_ = 1; break;
    case '<': ins.code_ = IROpCode::INS_ADP; ins.a_ = -1; break;
    }
    return is;
}

std::ostream& operator<<(std::ostream &os, const Instruction &ins) {
    const char *op = nullptr;
    switch (ins.code_) {
    case IROpCode::INS_ADD:     op = "INS_ADD"; break;
    case IROpCode::INS_ZERO:    op = "INS_ZERO"; break;
    case IROpCode::INS_ADP:     op = "INS_ADP"; break;
    case IROpCode::INS_ACL:     op = "INS_ACL"; break;
    case IROpCode::INS_IN:      op = "INS_IN"; break;
    case IROpCode::INS_OUT:     op = "INS_OUT"; break;
    case IROpCode::INS_LOOP:    op = "INS_LOOP"; break;
    case IROpCode::INS_END:     op = "INS_END"; break;
    case IROpCode::INS_INVALID: op = "INS_INVALID"; break;
    }
    return os << op
        << ' ' << ins.a_
        << ' ' << ins.b_
        << ' ' << ins.c_;
}
