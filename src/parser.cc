#include "arguments.hpp"
#include "asmbuf.hpp"
#include "error.hpp"
#include "ir.hpp"
#include "parser.hpp"

Parser::Parser(const Arguments &args) : verbose_{args.verbose} {}

void Parser::checkNotFinished() const {
    if (compiled_) {
        throw JITError("Parser used after compilation");
    }
}

void Parser::feed(std::istream &is) {
    checkNotFinished();
    while (is.good()) {
        Instruction ins;
        is >> ins;
        switch (ins.code_) {
        case IROpCode::INS_LOOP:
            ins.a_ = loopCount_;
            loopStack_.push(loopCount_);
            ++loopCount_;
            break;
        case IROpCode::INS_END_LOOP:
            if (loopStack_.size() == 0) {
                throw JITError("Unexpected ]");
            }
            ins.a_ = loopStack_.top();
            loopStack_.pop();
            break;
        default:
            break;
        }
        if (ins.code_ != IROpCode::INS_INVALID) {
            outStream_.emplace_back(std::move(ins));
        }
    }
}

std::vector<Instruction> Parser::compile() {
    checkNotFinished();
    if (loopStack_.size() > 0) {
        throw JITError("Unmatched [");
    }
    compiled_ = true;
    return std::move(outStream_);
}
