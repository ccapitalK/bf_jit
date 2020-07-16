#include <vector>

#include "arguments.hpp"
#include "interpreter.hpp"
#include "ir.hpp"
#include "runtime.hpp"

void interpret(const std::vector<Instruction> &prog, std::vector<char> &bfMem, const Arguments &) {
    const ssize_t BFMEM_LENGTH = bfMem.size();
    size_t dp{};

    std::vector<std::pair<uintptr_t, uintptr_t>> loopPositions;
    for (size_t i = 0; i < prog.size(); ++i) {
        const auto &ins = prog[i];
        switch (ins.code_) {
        case IROpCode::INS_LOOP:
            if (ins.a_ >= (ssize_t)loopPositions.size()) {
                loopPositions.resize(ins.a_ + 1);
            }
            loopPositions[ins.a_].first = i;
            break;
        case IROpCode::INS_END_LOOP:
            if (ins.a_ >= (ssize_t)loopPositions.size()) {
                loopPositions.resize(ins.a_ + 1);
            }
            loopPositions[ins.a_].second = i;
            break;
        default:
            break;
        }
    }
    for (size_t i = 0; i < prog.size(); ++i) {
        auto &ins = prog[i];
        switch (ins.code_) {
        case IROpCode::INS_ADD:
            bfMem[dp] += ins.a_;
            break;
        case IROpCode::INS_MUL: {
            auto remote = (dp + ins.a_) % BFMEM_LENGTH;
            bfMem[remote] += ins.b_ * bfMem[dp];
        } break;
        case IROpCode::INS_CONST:
            bfMem[dp] = ins.a_;
            break;
        case IROpCode::INS_ADP:
            dp = (dp + ins.a_) % BFMEM_LENGTH;
            break;
        case IROpCode::INS_IN:
            bfMem[dp] = mgetchar_0_on_eof();
            break;
        case IROpCode::INS_OUT:
            mputchar(bfMem[dp]);
            break;
        case IROpCode::INS_LOOP:
            if (bfMem[dp] == 0) {
                i = loopPositions[ins.a_].second;
            }
            break;
        case IROpCode::INS_END_LOOP:
            i = loopPositions[ins.a_].first - 1;
            break;
        default:
            throw JITError("ICE: Unhandled instruction");
        }
    }
}
