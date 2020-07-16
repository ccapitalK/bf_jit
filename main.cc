#include <ctime>
#include <fstream>
#include <iostream>
#include <stack>

#include "arguments.hpp"
#include "asmbuf.hpp"
#include "code_generator.hpp"
#include "error.hpp"
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

int main(int argc, char *argv[]) {
    Arguments arguments{argc, argv};
    try {
        run(arguments);
    } catch (JITError &e) {
        std::cout << "Fatal JITError caught: " << e.what() << '\n';
        return 1;
    }
}
