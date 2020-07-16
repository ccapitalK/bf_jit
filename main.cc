#include <ctime>
#include <fstream>
#include <iostream>
#include <stack>

#include <cxxopts.hpp>

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
        case IROpCode::INS_END_LOOP:
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

struct Arguments {
    size_t bfMemLength;
    std::vector<std::string> fileNames;
    bool verbose{false};
    bool dumpCode{false};
    bool genSyms{false};
    Arguments(int argc, char *argv[]) {
        cxxopts::Options options("bf_jit", "JIT-compiling interpeter for brainfuck");
        options.add_options()("m,mem-size", "Number of memory cells", cxxopts::value<size_t>()->default_value("32768"))(
            "file-names", "BF source file names",
            cxxopts::value<std::vector<std::string>>())("d,dump-code", "Dump the generated machine code")(
            "g,gen-syms", "Generate jit symbol maps for debugging purposes")("h,help", "Print help")(
            "v,verbose", "Print more information");
        options.positional_help("[input files]").show_positional_help();
        options.parse_positional({"file-names"});
        try {
            auto result = options.parse(argc, argv);
            if (result.count("help")) {
                std::cout << options.help() << '\n';
                exit(0);
            }
            if (result.count("verbose")) {
                verbose = true;
            }
            if (result.count("gen-syms")) {
                genSyms = true;
            }
            if (result.count("dump-code")) {
                dumpCode = true;
            }
            bfMemLength = result["mem-size"].as<size_t>();
            if (!result.count("file-names")) {
                throw cxxopts::OptionException("No source files specified");
            }
            fileNames = result["file-names"].as<std::vector<std::string>>();
        } catch (cxxopts::OptionException &e) {
            std::cout << options.help() << '\n';
            std::cout << "Failed to parse args: " << e.what() << '\n';
            exit(1);
        }
    }
};

void run(const Arguments &arguments) {
    std::ifstream in;
    in.rdbuf()->pubsetbuf(rdbuf, RDBUF_SIZE);

    time();
    Parser parser;
    for (auto &fileName : arguments.fileNames) {
        in.open(fileName);
        if (!in.good()) {
            throw JITError("Failed to open file \"", fileName, "\"");
        }
        parser.feed(in);
    }
    std::vector<char> bfMem(arguments.bfMemLength, 0);
    auto prog = parser.compile();
    CodeGenerator codeGenerator(bfMem, arguments.genSyms);
    auto offset = codeGenerator.compile(prog);
    if (arguments.verbose) {
        std::cout << "Compiled in " << time() << " seconds\n";
        std::cout << "Used " << codeGenerator.generatedLength() << " bytes\n";
        std::cout << "Running with mem-size: " << arguments.bfMemLength << " bytes\n";
    }
    if (arguments.dumpCode) {
        std::cout << "Instructions : " << codeGenerator.instructionHexDump() << '\n';
    }

    time();
    codeGenerator.enter(offset);
    if (arguments.verbose) {
        std::cout << '\n';
        std::cout << "Executed in " << time() << " seconds\n";
    }
}

int main(int argc, char *argv[]) {
    Arguments arguments{argc, argv};
    try {
        run(arguments);
    } catch (JITError &e) {
        std::cout << "Fatal JITError caught: " << e.what() << '\n';
        return 1;
    }
}
