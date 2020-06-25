#pragma once

#include <iostream>
#include <optional>
#include <unordered_map>
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
        auto sawChange{false};
        std::optional<decltype(outStream_)::iterator> foldStart{};
        for (auto pos = outStream_.begin(); pos != outStream_.end(); ++pos) {
            auto code = pos->code_;
            if (code == IROpCode::INS_ADD || code == IROpCode::INS_ADP) {
                if (foldStart.has_value() && (*foldStart)->code_ == code) {
                    (*foldStart)->a_ += pos->a_;
                    pos->code_ = IROpCode::INS_INVALID;
                } else {
                    foldStart = pos;
                }
            } else {
                foldStart = {};
            }
        }
        return sawChange;
    }
    bool deadCodeEliminationPass() {
        checkNotFinished();
        if (outStream_.size() == 0) {
            return false;
        }
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
        for(auto i = 0u; i < outStream_.size(); ++i) {
            if (outStream_[i].code_ == IROpCode::INS_INVALID) {
                outStream_.resize(i);
                return true;
            }
        }
        return false;
    }
    bool loopFoldPass() {
        checkNotFinished();
        if (outStream_.size() < 3) {
            return false;
        }
        bool rv{false};
        for(auto i = 0u; i + 2 < outStream_.size(); ++i) {
            auto &c1 = outStream_[i].code_;
            auto &c2 = outStream_[i+1].code_;
            auto &c3 = outStream_[i+2].code_;
            if (c1 == IROpCode::INS_LOOP && c2 == IROpCode::INS_ADD && c3 == IROpCode::INS_END) {
                rv = true;
                c1 = IROpCode::INS_ZERO;
                c2 = IROpCode::INS_INVALID;
                c3 = IROpCode::INS_INVALID;
            }
        }
        return rv;
    }
    std::vector<Instruction> compile() {
        checkNotFinished();
        if (loopStack_.size() > 0) {
            throw JITError("Unmatched [");
        }
        while (constPropagatePass() || deadCodeEliminationPass() || loopFoldPass());
        compiled_ = true;
        return std::move(outStream_);
    }
};

