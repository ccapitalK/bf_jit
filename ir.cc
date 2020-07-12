#include "ir.hpp"

std::istream &operator>>(std::istream &is, Instruction &ins) {
    ins.code_ = IROpCode::INS_INVALID;
    char opc;
    is >> opc;
    if (!is.good())
        return is;
    switch (opc) {
    case ',':
        ins.code_ = IROpCode::INS_IN;
        break;
    case '.':
        ins.code_ = IROpCode::INS_OUT;
        break;
    case '[':
        ins.code_ = IROpCode::INS_LOOP;
        break;
    case ']':
        ins.code_ = IROpCode::INS_END;
        break;
    case '+':
        ins.code_ = IROpCode::INS_ADD;
        ins.a_ = 1;
        break;
    case '-':
        ins.code_ = IROpCode::INS_ADD;
        ins.a_ = -1;
        break;
    case '>':
        ins.code_ = IROpCode::INS_ADP;
        ins.a_ = 1;
        break;
    case '<':
        ins.code_ = IROpCode::INS_ADP;
        ins.a_ = -1;
        break;
    }
    return is;
}

std::ostream &operator<<(std::ostream &os, const Instruction &ins) {
    const char *op = nullptr;
    switch (ins.code_) {
    case IROpCode::INS_ADD:
        op = "INS_ADD";
        break;
    case IROpCode::INS_MUL:
        op = "INS_MUL";
        break;
    case IROpCode::INS_ZERO:
        op = "INS_ZERO";
        break;
    case IROpCode::INS_ADP:
        op = "INS_ADP";
        break;
    case IROpCode::INS_IN:
        op = "INS_IN";
        break;
    case IROpCode::INS_OUT:
        op = "INS_OUT";
        break;
    case IROpCode::INS_LOOP:
        op = "INS_LOOP";
        break;
    case IROpCode::INS_END:
        op = "INS_END";
        break;
    case IROpCode::INS_INVALID:
        op = "INS_INVALID";
        break;
    }
    return os << op << ' ' << ins.a_ << ' ' << ins.b_ << ' ' << ins.c_;
}
