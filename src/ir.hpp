#pragma once

#include <iostream>

#include "error.hpp"

enum class IROpCode {
    ADD,      // Add to current cell
    MUL,      // v[dp+a_] += v[dp] * b_
    CONST,    // Set current cell to a_
    ADP,      // Add to data pointer
    IN,       // call mgetc()
    OUT,      // call mputc()
    LOOP,     // Start of loop
    END_LOOP, // End of loop
    INVALID   // Not a valid instruction
};

std::ostream &operator<<(std::ostream &os, IROpCode code);

struct Instruction {
    IROpCode code_{};
    int a_{};
    int b_{};
    int c_{};
    Instruction() : code_{IROpCode::INVALID} {}
    Instruction(IROpCode code) : code_(code), a_(0), b_(0), c_(0) {}
    Instruction(IROpCode code, int a) : code_(code), a_(a), b_(0), c_(0) {}
    Instruction(IROpCode code, int a, int b) : code_(code), a_(a), b_(b), c_(0) {}
    Instruction(IROpCode code, int a, int b, int c) : code_(code), a_(a), b_(b), c_(c) {}
    bool isFoldable() { return code_ == IROpCode::ADD || code_ == IROpCode::ADP; }
    friend std::istream &operator>>(std::istream &is, Instruction &ins);
    friend std::ostream &operator<<(std::ostream &os, const Instruction &ins);
};
