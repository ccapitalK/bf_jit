#include "ir.hpp"

std::istream &operator>>(std::istream &is, Instruction &ins) {
    ins.code_ = IROpCode::INVALID;
    char opc;
    is >> opc;
    if (!is.good())
        return is;
    switch (opc) {
    case ',':
        ins.code_ = IROpCode::IN;
        break;
    case '.':
        ins.code_ = IROpCode::OUT;
        break;
    case '[':
        ins.code_ = IROpCode::LOOP;
        break;
    case ']':
        ins.code_ = IROpCode::END_LOOP;
        break;
    case '+':
        ins.code_ = IROpCode::ADD;
        ins.a_ = 1;
        break;
    case '-':
        ins.code_ = IROpCode::ADD;
        ins.a_ = -1;
        break;
    case '>':
        ins.code_ = IROpCode::ADP;
        ins.a_ = 1;
        break;
    case '<':
        ins.code_ = IROpCode::ADP;
        ins.a_ = -1;
        break;
    }
    return is;
}

std::ostream &operator<<(std::ostream &os, IROpCode code) {
    const char *op = nullptr;
    switch (code) {
    case IROpCode::ADD:
        op = "ADD";
        break;
    case IROpCode::MUL:
        op = "MUL";
        break;
    case IROpCode::CONST:
        op = "CONST";
        break;
    case IROpCode::ADP:
        op = "ADP";
        break;
    case IROpCode::IN:
        op = "IN";
        break;
    case IROpCode::OUT:
        op = "OUT";
        break;
    case IROpCode::LOOP:
        op = "LOOP";
        break;
    case IROpCode::END_LOOP:
        op = "END_LOOP";
        break;
    case IROpCode::INVALID:
        op = "INVALID";
        break;
    }
    return os << op;
}

std::ostream &operator<<(std::ostream &os, const Instruction &ins) {
    return os << ins.code_ << ' ' << ins.a_ << ' ' << ins.b_ << ' ' << ins.c_;
}
