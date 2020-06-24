#pragma once

#include <iostream>
#include <vector>

#include "asmbuf.hpp"
#include "error.hpp"
#include "ir.hpp"

//ASMBuf compile_bf(std::vector<Instruction> &is);

class Parser {
    std::vector<Instruction> outStream_;
    std::stack<int> loopStack_;
    size_t loopCount_{};
    bool compiled_{false};
public:
    void checkNotFinished() const {
        if (compiled_) {
            throw JITError("Parser used after compilation");
        }
    }
    void feed(std::istream& is) {
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
            case IROpCode::INS_END:
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
    bool constPropagatePass() {
        checkNotFinished();
        if (outStream_.size() == 0) {
            return false;
        }
        auto sawChange{false};
        auto p1 = outStream_.begin();
        auto p2 = p1+1;
        while (p2 != outStream_.end()) {
            auto code = p1->code_;
            if (code == p2->code_ && (code == IROpCode::INS_ADD || code == IROpCode::INS_ADP)) {
                sawChange = true;
                p1->a_ += p2->a_;
                p2->code_ = IROpCode::INS_INVALID;
                ++p2;
            } else {
                p1 = p2;
                ++p2;
            }
        }
        return sawChange;
    }
    bool deadCodeEliminationPass() {
        checkNotFinished();
        if (outStream_.size() == 0) {
            return false;
        }
        auto sawChange{false};
        auto p1 = outStream_.begin();
        auto p2 = p1+1;
        while (p2 != outStream_.end()) {
            if (p1->code_ != IROpCode::INS_INVALID) {
                ++p1;
                if (p2 == p1) {
                    ++p2;
                }
            } else {
                if (p2->code_ != IROpCode::INS_INVALID) {
                    *p1 = *p2;
                    p2->code_ = IROpCode::INS_INVALID;
                } else {
                    ++p2;
                }
            }
        }
        for(auto i = 0; i < outStream_.size(); ++i) {
            if (outStream_[i].code_ == IROpCode::INS_INVALID) {
                outStream_.resize(i);
                return true;
            }
        }
        return false;
    }
    std::vector<Instruction> compile() {
        checkNotFinished();
        if (loopStack_.size() > 0) {
            throw JITError("Unmatched [");
        }
        while (constPropagatePass() || deadCodeEliminationPass());
        compiled_ = true;
        return std::move(outStream_);
    }
};

