#include <functional>
#include <vector>

#include "arguments.hpp"
#include "interpreter.hpp"
#include "ir.hpp"
#include "runtime.hpp"

template <typename CellType>
void interpret(const std::vector<Instruction> &prog, std::vector<CellType> &bfMem, const Arguments &args) {
    const ssize_t BFMEM_LENGTH = bfMem.size();
    size_t dp{};
    auto mputchar = putCharFunc(args);
    auto mGetCharFunc = getCharFunc(args);
    std::function<int()> mgetchar = [&]() { return mGetCharFunc(0); };
    if (args.getCharBehaviour == GetCharBehaviour::EOF_DOESNT_MODIFY) {
        mgetchar = [&]() { return mGetCharFunc(bfMem[dp]); };
    }

    std::vector<std::pair<uintptr_t, uintptr_t>> loopPositions;
    for (size_t i = 0; i < prog.size(); ++i) {
        const auto &ins = prog[i];
        switch (ins.code_) {
        case IROpCode::LOOP:
            if (ins.a_ >= (ssize_t)loopPositions.size()) {
                loopPositions.resize(ins.a_ + 1);
            }
            loopPositions[ins.a_].first = i;
            break;
        case IROpCode::END_LOOP:
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
        case IROpCode::ADD:
            bfMem[dp] += ins.a_;
            break;
        case IROpCode::MUL: {
            auto remote = (dp + ins.a_) % BFMEM_LENGTH;
            bfMem[remote] += ins.b_ * bfMem[dp];
        } break;
        case IROpCode::CONST:
            bfMem[dp] = ins.a_;
            break;
        case IROpCode::ADP:
            dp = (dp + ins.a_) % BFMEM_LENGTH;
            break;
        case IROpCode::IN:
            bfMem[dp] = mgetchar();
            break;
        case IROpCode::OUT:
            mputchar(bfMem[dp] & 0xff);
            break;
        case IROpCode::LOOP:
            if (bfMem[dp] == 0) {
                i = loopPositions[ins.a_].second;
            }
            break;
        case IROpCode::END_LOOP:
            i = loopPositions[ins.a_].first - 1;
            break;
        default:
            throw JITError("ICE: Unhandled instruction");
        }
    }
}

template void interpret(const std::vector<Instruction> &prog, std::vector<char> &bfMem, const Arguments &args);
template void interpret(const std::vector<Instruction> &prog, std::vector<short> &bfMem, const Arguments &args);
template void interpret(const std::vector<Instruction> &prog, std::vector<int> &bfMem, const Arguments &args);
