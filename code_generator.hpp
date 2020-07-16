#pragma once

#include <fstream>
#include <string>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include "arguments.hpp"
#include "asmbuf.hpp"
#include "error.hpp"
#include "ir.hpp"

template <typename T> bool is_pow_2(T v) {
    size_t nonZeroBits = 0;
    for (; v != 0; v >>= 1) {
        nonZeroBits += v & 1;
    }
    return nonZeroBits == 1;
}
template <typename T> size_t log_2(T v) {
    if (v == 0)
        return 0;
    size_t seenBits = 0;
    while ((v & 1) == 0) {
        ++seenBits;
        v >>= 1;
    }
    return seenBits;
}

class CodeGenerator {
  private:
    void generatePrelude();
    void generateInsAdd(unsigned char step);
    void generateInsAdp(int step);
    void generateInsEndLoop(int loopNumber);
    void generateInsIn();
    void generateInsLoop(int loopNumber);
    void generateInsMul(int offset, char multFactor);
    void generateInsOut();
    void generateInsConst(int constant);
    void generateEpilogue();
    ASMBuf buf_{4};
    std::vector<char> &bfMem_;
    std::unordered_map<size_t, std::pair<uintptr_t, uintptr_t>> loopStarts_;
    // If this isn't signed, calculations in the code
    // generator default to unsigned, and would require a lot of casting
    const ssize_t BFMEM_LENGTH{(ssize_t)bfMem_.size()};
    const bool IS_POW_2_MEM_LENGTH{is_pow_2(BFMEM_LENGTH)};
    const bool genPerfMap_{false};
    std::ofstream perfSymbolMap_;

  public:
    CodeGenerator(std::vector<char> &bfMem, const Arguments &args) : bfMem_{bfMem}, genPerfMap_{args.genSyms} {
        if (genPerfMap_) {
            size_t pid = getpid();
            std::stringstream ss;
            ss << "/tmp/perf-" << pid << ".map";
            perfSymbolMap_.open(ss.str());
        }
    }
    ASMBufOffset compile(const std::vector<Instruction> &prog);
    void enter(ASMBufOffset);
    std::string instructionHexDump() const;
    size_t generatedLength() const;
};
