#pragma once

#include <fstream>
#include <unordered_map>
#include <string>
#include <unistd.h>
#include <vector>

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
    void generateInsZero();
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
    CodeGenerator(std::vector<char> &bfMem) : bfMem_{bfMem} {
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
