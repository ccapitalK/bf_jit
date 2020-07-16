#include <ctime>
#include <fstream>
#include <iostream>
#include <stack>

#include "arguments.hpp"
#include "asmbuf.hpp"
#include "code_generator.hpp"
#include "error.hpp"
#include "interpreter.hpp"
#include "parse.hpp"
#include "runtime.hpp"

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

template <typename CellType>
void run(const Arguments &arguments) {
    std::ifstream in;
    in.rdbuf()->pubsetbuf(rdbuf, RDBUF_SIZE);

    time();
    Parser parser{arguments};
    for (auto &fileName : arguments.fileNames) {
        in.open(fileName);
        if (!in.good()) {
            throw JITError("Failed to open file \"", fileName, "\"");
        }
        parser.feed(in);
    }
    std::vector<CellType> bfMem(arguments.bfMemLength, 0);
    auto prog = parser.compile();
    if (arguments.useInterpreter) {
        if (arguments.verbose) {
            std::cout << "Compiled in " << time() << " seconds\n";
        }
        time();
        interpret(prog, bfMem, arguments);
        if (arguments.verbose) {
            std::cout << '\n';
            std::cout << "Executed in " << time() << " seconds\n";
        }
    } else {
        CodeGenerator codeGenerator(bfMem, arguments);
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
    if (arguments.dumpMem) {
        std::cout << "Mem: ";
        for (auto i = 0u; i < std::min((size_t)32, bfMem.size()); ++i) {
            std::cout << (int)bfMem[i] << ' ';
        }
        std::cout << '\n';
    }
}

int main(int argc, char *argv[]) {
    Arguments arguments{argc, argv};
    try {
        switch(arguments.cellBitWidth) {
        case 8:
            run<char>(arguments);
            break;
        case 16:
            run<short>(arguments);
            break;
        case 32:
            run<int>(arguments);
            break;
        };
    } catch (JITError &e) {
        std::cout << "Fatal JITError caught: " << e.what() << '\n';
        return 1;
    }
}
