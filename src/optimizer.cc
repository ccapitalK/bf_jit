#include <map>
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

struct ConstFoldable {
    int val{};
    enum class Type : int {
        Add,
        Const,
    } type{Type::Add};
    Instruction genIns() const {
        return {
            type == Type::Add ? IROpCode::ADD : IROpCode::CONST,
            val
        };
    }
};

// For each run of Add, Const, Adp, re-organize them
bool Optimizer::constPropagatePass() {
    auto sawChange{false};
    if (prog().size() == 0) {
        return false;
    }
    std::map<int, ConstFoldable> constants;
    int offset{};
    auto finalize = [&](size_t start, size_t end) {
        int tapePosition{};
        size_t codePosition{start};
        if (offset != 0 && constants.count(0)) {
            prog()[codePosition++] = constants[0].genIns();
        }
        for (auto [off, fold]: constants) {
            if (off != 0 && off != offset) {
                prog()[codePosition++] = {IROpCode::ADP, off - tapePosition};
                prog()[codePosition++] = fold.genIns();
                tapePosition = off;
            }
        }
        if (tapePosition != offset) {
            prog()[codePosition++] = {IROpCode::ADP, offset - tapePosition};
        }
        if (constants.count(offset)) {
            prog()[codePosition++] = constants[offset].genIns();
        }
        for (; codePosition < end; ++codePosition) {
            prog()[codePosition] = IROpCode::INVALID;
        }
    };
    size_t foldStart = 0;
    for (size_t pos = 0; pos != prog().size(); ++pos) {
        auto &ins = prog()[pos];
        switch (ins.code_) {
        case IROpCode::ADD: constants[offset].val += ins.a_; break;
        case IROpCode::ADP: offset += ins.a_; break;
        case IROpCode::CONST: constants[offset] = {ins.a_, ConstFoldable::Type::Const}; break;
        default:
            if (offset != 0 || constants.size()) {
                finalize(foldStart, pos);
                offset = {};
                constants = {};
            }
            foldStart = pos + 1;
            break;
        }
    }
    finalize(foldStart, prog().size());
    return sawChange;
}

bool Optimizer::deadCodeEliminationPass() {
    auto sawChange{false};
    auto writePos = prog().begin();
    for (auto &v : prog()) {
        if (v.code_ == IROpCode::INVALID) {
            sawChange = true;
        } else if ((v.code_ == IROpCode::ADD || v.code_ == IROpCode::ADP) && v.a_ == 0) {
            sawChange = true;
        } else {
            *(writePos++) = v;
        }
    }
    prog().resize(std::distance(prog().begin(), writePos));
    return sawChange;
}

bool Optimizer::multPass() {
    std::optional<size_t> loopStartPosition{};
    std::unordered_map<int, int> relativeAdds;
    ssize_t currOffset{};
    int origModBy{};
    auto sawChange{false};
    for (size_t i = 0u; i < prog().size(); ++i) {
        auto &ins = prog()[i];
        switch (ins.code_) {
        case IROpCode::ADD:
            if (currOffset == 0) {
                origModBy += ins.a_;
            } else if (loopStartPosition.has_value()) {
                relativeAdds[currOffset] += ins.a_;
            }
            break;
        case IROpCode::ADP:
            currOffset += ins.a_;
            break;
        case IROpCode::INVALID:
            break;
        case IROpCode::LOOP:
            loopStartPosition = i;
            currOffset = 0;
            origModBy = 0;
            relativeAdds = {};
            break;
        case IROpCode::END_LOOP: {
            if (loopStartPosition.has_value() && std::abs(origModBy) == 1 && currOffset == 0) {
                sawChange = true;
                auto writeIndex = *loopStartPosition, end = i + 1;
                for (auto [x, v] : relativeAdds) {
                    if (v != 0) {
                        prog()[writeIndex++] = Instruction{IROpCode::MUL, x, -v * origModBy};
                    }
                }
                prog()[writeIndex++] = Instruction{IROpCode::CONST, 0};
                for (; writeIndex < end; ++writeIndex) {
                    prog()[writeIndex].code_ = IROpCode::INVALID;
                }
            }
            loopStartPosition = {};
            break;
        }
        default:
            loopStartPosition = {};
        }
    }
    return sawChange;
}

bool Optimizer::optimizePass() {
    // Explicit, to avoid short circuit eval
    auto sawChange = false;
    sawChange = constPropagatePass() || sawChange;
    sawChange = deadCodeEliminationPass() || sawChange;
    sawChange = multPass() || sawChange;
    return sawChange;
}
