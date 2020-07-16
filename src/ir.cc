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
        ins.code_ = IROpCode::INS_END_LOOP;
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

std::ostream &operator<<(std::ostream &os, IROpCode code) {
    const char *op = nullptr;
    switch (code) {
    case IROpCode::INS_ADD:
        op = "INS_ADD";
        break;
    case IROpCode::INS_MUL:
        op = "INS_MUL";
        break;
    case IROpCode::INS_CONST:
        op = "INS_CONST";
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
    case IROpCode::INS_END_LOOP:
        op = "INS_END_LOOP";
        break;
    case IROpCode::INS_INVALID:
        op = "INS_INVALID";
        break;
    }
    return os << op;
}

std::ostream &operator<<(std::ostream &os, const Instruction &ins) {
    return os << ins.code_ << ' ' << ins.a_ << ' ' << ins.b_ << ' ' << ins.c_;
}
