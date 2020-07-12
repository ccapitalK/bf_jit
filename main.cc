#include <ctime>
#include <fstream>
#include <iostream>
#include <stack>

#include "asmbuf.hpp"
#include "code_generator.hpp"
#include "error.hpp"
#include "parse.hpp"
#include "runtime.hpp"

void interpret(const std::vector<Instruction> &prog, std::vector<char> &bfMem) {
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
        case IROpCode::INS_END:
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
        case IROpCode::INS_ZERO:
            bfMem[dp] = 0;
            break;
        case IROpCode::INS_ADP:
            dp = (dp + ins.a_) % BFMEM_LENGTH;
            break;
        case IROpCode::INS_IN:
            bfMem[dp] = mgetc();
            break;
        case IROpCode::INS_OUT:
            mputchar(bfMem[dp]);
            break;
        case IROpCode::INS_LOOP:
            if (bfMem[dp] == 0) {
                i = loopPositions[ins.a_].second;
            }
            break;
        case IROpCode::INS_END:
            i = loopPositions[ins.a_].first - 1;
            break;
        default:
            throw JITError("ICE: Unhandled instruction");
        }
    }
}

double time() {
    static std::clock_t startTime = std::clock();
    std::clock_t now = std::clock();
    double duration = (now - startTime) / (double)CLOCKS_PER_SEC;
    startTime = now;
    return duration;
}

// Optimization to minimize number of read() calls made by ifstream
constexpr size_t RDBUF_SIZE = 256 * 1024;
static char rdbuf[RDBUF_SIZE];

int main(int argc, const char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " infile\n";
        return 1;
    }
    try {
        const int BFMEM_LENGTH = 4096;
        std::ifstream in;
        in.rdbuf()->pubsetbuf(rdbuf, RDBUF_SIZE);
        in.open(argv[1]);
        if (!in.good()) {
            throw JITError("Failed to open file");
        }

        time();
        Parser parser;
        parser.feed(in);
        std::vector<char> bfMem(BFMEM_LENGTH, 0);
        auto prog = parser.compile();
        CodeGenerator codeGenerator(bfMem);
        auto offset = codeGenerator.compile(prog);
        std::cout << "Compiled in " << time() << " seconds\n";
        std::cout << "Used " << codeGenerator.generatedLength() << " bytes\n";
        if (0) {
            std::cout << "Hex: " << codeGenerator.instructionHexDump() << '\n';
        }

        time();
        codeGenerator.enter(offset);
        std::cout << '\n';
        std::cout << "Executed in " << time() << " seconds\n";
        return 0;
    } catch (JITError &e) {
        std::cout << "Fatal JITError caught: " << e.what() << '\n';
        return 1;
    }
}
