#include <cassert>
#include <optional>
#include <unordered_map>

#include "optimizer.hpp"

Optimizer::Optimizer(const Arguments &arguments) : verbose_{arguments.verbose} {}

void Optimizer::optimize(std::vector<Instruction> &program) {
    progPtr_ = &program;
    size_t numOptPasses = 1;
    while (optimizePass()) {
        ++numOptPasses;
    }
    if (verbose_) {
        std::cout << "Optimized " << numOptPasses << " times\n";
    }
    progPtr_ = nullptr;
}

std::vector<Instruction> &Optimizer::prog() { return *progPtr_; }

bool Optimizer::constPropagatePass() {
    auto sawChange{false};
    if (prog().size() == 0) {
        return false;
    }
    for (auto foldStart = prog().begin(), pos = foldStart + 1; pos != prog().end(); ++pos) {
        if (pos->isFoldable() && foldStart->code_ == pos->code_) {
            foldStart->a_ += pos->a_;
            pos->code_ = IROpCode::INS_INVALID;
        } else {
            foldStart = pos;
        }
    }
    return sawChange;
}

bool Optimizer::deadCodeEliminationPass() {
    auto sawChange{false};
    auto writePos = prog().begin();
    for (auto &v : prog()) {
        if (v.code_ == IROpCode::INS_INVALID) {
            sawChange = true;
        } else if ((v.code_ == IROpCode::INS_ADD || v.code_ == IROpCode::INS_ADP) && v.a_ == 0) {
            sawChange = true;
        } else {
            *(writePos++) = v;
        }
    }
    prog().resize(std::distance(prog().begin(), writePos));
    return sawChange;
}

void Optimizer::rewriteMultLoop(size_t start, size_t end) {
    // A map of relative offsets to the amount we change them by each loop iteration
    std::unordered_map<int, int> relativeAdds;
    // The current offset from the original cell
    auto currOffset = 0;
    // The amount we change the origin cell by each loop iteration
    auto originAdd = 0;
    for (auto i = start + 1; i < end - 1; ++i) {
        auto &ins = prog()[i];
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
            prog()[writeIndex++] = Instruction{IROpCode::INS_MUL, x, v * origMultFactor};
        }
    }
    prog()[writeIndex++] = Instruction{IROpCode::INS_CONST, 0};
    while (writeIndex < end)
        prog()[writeIndex++].code_ = IROpCode::INS_INVALID;
}

bool Optimizer::makeMultPass() {
    std::optional<size_t> loopStart{};
    ssize_t currOffset{};
    ssize_t origModBy{};
    auto sawChange{false};
    for (auto i = 0u; i < prog().size(); ++i) {
        auto &ins = prog()[i];
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

bool Optimizer::optimizePass() {
    // Explicit, to avoid short circuit eval
    auto sawChange = false;
    sawChange = constPropagatePass() || sawChange;
    sawChange = deadCodeEliminationPass() || sawChange;
    sawChange = makeMultPass() || sawChange;
    return sawChange;
}
