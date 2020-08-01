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

bool Parser::constPropagatePass() {
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

bool Parser::deadCodeEliminationPass() {
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

void Parser::rewriteMultLoop(size_t start, size_t end) {
    // A map of relative offsets to the amount we change them by each loop iteration
    std::unordered_map<int, int> relativeAdds;
    // The current offset from the original cell
    auto currOffset = 0;
    // The amount we change the origin cell by each loop iteration
    auto originAdd = 0;
    for (auto i = start + 1; i < end - 1; ++i) {
        auto &ins = outStream_[i];
        switch (ins.code_) {
        case IROpCode::INS_ADD:
            if (currOffset != 0) {
                relativeAdds[currOffset] += ins.a_;
            } else {
                originAdd += ins.a_;
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
    assert(std::abs(originAdd) == 1);
    const auto origMultFactor = -originAdd;
    auto writeIndex = start;
    for (auto [x, v] : relativeAdds) {
        if (v != 0) {
            outStream_[writeIndex++] = Instruction{IROpCode::INS_MUL, x, v * origMultFactor};
        }
    }
    outStream_[writeIndex++] = Instruction{IROpCode::INS_CONST, 0};
    while (writeIndex < end)
        outStream_[writeIndex++].code_ = IROpCode::INS_INVALID;
}

bool Parser::makeMultPass() {
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
        case IROpCode::INS_END_LOOP:
            if (loopStart.has_value() && std::abs(origModBy) == 1 && currOffset == 0) {
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

bool Parser::optimize() {
    // Explicit, to avoid short circuit eval
    auto sawChange = false;
    sawChange = constPropagatePass() || sawChange;
    sawChange = deadCodeEliminationPass() || sawChange;
    sawChange = makeMultPass() || sawChange;
    return sawChange;
}

std::vector<Instruction> Parser::compile() {
    checkNotFinished();
    if (loopStack_.size() > 0) {
        throw JITError("Unmatched [");
    }
    size_t numOptPasses = 1;
    while (optimize()) {
        ++numOptPasses;
    }
    if (verbose_) {
        std::cout << "Optimized " << numOptPasses << " times\n";
    }
    compiled_ = true;
    return std::move(outStream_);
}
