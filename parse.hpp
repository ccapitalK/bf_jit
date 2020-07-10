#pragma once

#include <iostream>
#include <optional>
#include <stack>
#include <unordered_map>
#include <vector>

#include "asmbuf.hpp"
#include "error.hpp"
#include "ir.hpp"

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
    void feed(std::istream &is) {
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
        if (outStream_.size() == 0) {
            return false;
        }
        for (auto foldStart = outStream_.begin(), pos = foldStart + 1; pos != outStream_.end(); ++pos) {
            if (pos->isFoldable() && foldStart->code_ == pos->code_) {
                foldStart->a_ += pos->a_;
                pos->code_ = IROpCode::INS_INVALID;
            } else {
                foldStart = pos;
            }
        }
        return sawChange;
    }
    bool deadCodeEliminationPass() {
        checkNotFinished();
        auto sawChange{false};
        auto writePos = outStream_.begin();
        for (auto &v : outStream_) {
            if (v.code_ == IROpCode::INS_INVALID) {
                sawChange = true;
            } else if ((v.code_ == IROpCode::INS_ADD || v.code_ == IROpCode::INS_ADP) && v.a_ == 0) {
                sawChange = true;
            } else {
                *(writePos++) = v;
            }
        }
        outStream_.resize(std::distance(outStream_.begin(), writePos));
        return sawChange;
    }
    // Pre: the code at [start...end) contains a loop (including []), that can be converted into mults
    //      i.e. balanced count of < and >, single - at root, only invalid/add/adp
    void rewriteMultLoop(size_t start, size_t end) {
        // std::cout << "Making MUL at [" << start << ',' << end << ")\n";
        // a map of relative offsets to the amount we change them by each loop iteration
        std::unordered_map<int, int> relativeAdds;
        auto currOffset = 0;
#if 0
        if (outStream_[start].code_ != IROpCode::INS_LOOP || outStream_[end-1].code_ != IROpCode::INS_END) {
            throw JITError("invalid loop area");
        }
#endif
        for (auto i = start + 1; i < end - 1; ++i) {
            auto &ins = outStream_[i];
            switch (ins.code_) {
            case IROpCode::INS_ADD:
                if (currOffset != 0) {
                    relativeAdds[currOffset] += ins.a_;
                }
                break;
            case IROpCode::INS_ADP:
                currOffset += ins.a_;
                break;
            case IROpCode::INS_INVALID:
                break;
            default:
                throw JITError("ICE: Unexpected instruction");
            }
        }
#if 0
        if (currOffset != 0) {
            throw JITError("invalid loop area");
        }
#endif
        auto writeIndex = start;
        for (auto [x, v] : relativeAdds) {
            if (v != 0) {
                outStream_[writeIndex++] = Instruction{IROpCode::INS_MUL, x, v};
            }
        }
        outStream_[writeIndex++] = Instruction{IROpCode::INS_ZERO};
        while (writeIndex < end)
            outStream_[writeIndex++].code_ = IROpCode::INS_INVALID;
    }
    bool makeMultPass() {
        checkNotFinished();
        std::optional<size_t> loopStart{};
        ssize_t currOffset{};
        ssize_t origModBy{};
        auto sawChange{false};
        for (auto i = 0u; i < outStream_.size(); ++i) {
            auto &ins = outStream_[i];
            switch (ins.code_) {
            case IROpCode::INS_ADD:
                if (currOffset == 0) {
                    origModBy += ins.a_;
                }
                break;
            case IROpCode::INS_ADP:
                currOffset += ins.a_;
                break;
            case IROpCode::INS_INVALID:
                break;
            case IROpCode::INS_LOOP:
                loopStart = i;
                currOffset = 0;
                origModBy = 0;
                break;
            case IROpCode::INS_END:
                if (loopStart.has_value() && origModBy == -1 && currOffset == 0) {
                    sawChange = true;
                    rewriteMultLoop(*loopStart, i + 1);
                }
                loopStart = {};
                break;
            default:
                loopStart = {};
            }
        }
        return sawChange;
    }
    bool optimize() {
        // Explicit, to avoid short circuit eval
        auto v = false;
        v = constPropagatePass() || v;
        v = deadCodeEliminationPass() || v;
        v = makeMultPass() || v;
        return v;
    }
    std::vector<Instruction> compile() {
        checkNotFinished();
        if (loopStack_.size() > 0) {
            throw JITError("Unmatched [");
        }
        size_t numOptPasses = 1;
        while (optimize())
            ++numOptPasses;
        std::cout << "Optimized " << numOptPasses << " times\n";
        compiled_ = true;
        return std::move(outStream_);
    }
};
