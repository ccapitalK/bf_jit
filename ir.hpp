#pragma once

#include <iostream>

#include "error.hpp"

enum class IROpCode {
    INS_ADD,    // Add to current cell
    INS_MUL,    // v[dp+a_] += v[dp] * b_
    INS_ZERO,   // Set current cell to zero
    INS_ADP,    // Add to data pointer
    INS_IN,     // call mgetc()
    INS_OUT,    // call mputc()
    INS_LOOP,   // Start of loop
    INS_END_LOOP,    // End of loop
    INS_INVALID // Not a valid instruction
};

std::ostream &operator<<(std::ostream &os, IROpCode code);

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
    bool isFoldable() { return code_ == IROpCode::INS_ADD || code_ == IROpCode::INS_ADP; }
    friend std::istream &operator>>(std::istream &is, Instruction &ins);
    friend std::ostream &operator<<(std::ostream &os, const Instruction &ins);
};
