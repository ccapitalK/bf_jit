#include <ctime>
#include <fstream>

#include "arguments.hpp"
#include "code_generator.hpp"
#include "engine.hpp"
#include "interpreter.hpp"

template <typename CellType>
Engine<CellType>::Engine(const Arguments &arguments)
    : rdbuf_(RDBUF_SIZE, 0), arguments_{arguments}, bfMem_(arguments_.bfMemLength, 0) {}

static double time() {
    static std::clock_t startTime = std::clock();
    std::clock_t now = std::clock();
    double duration = (now - startTime) / (double)CLOCKS_PER_SEC;
    startTime = now;
    return duration;
}

template <typename CellType> void Engine<CellType>::run() {
    std::ifstream in;
    in.rdbuf()->pubsetbuf(rdbuf_.data(), RDBUF_SIZE);

    time();
    for (auto &fileName : arguments_.fileNames) {
        in.open(fileName);
        if (!in.good()) {
            throw JITError("Failed to open file \"", fileName, "\"");
        }
        parser_.feed(in);
    }
    auto prog = parser_.compile();
    if (arguments_.useInterpreter) {
        if (arguments_.verbose) {
            std::cout << "Compiled in " << time() << " seconds\n";
        }
        time();
        interpret(prog, bfMem_, arguments_);
        if (arguments_.verbose) {
            std::cout << '\n';
            std::cout << "Executed in " << time() << " seconds\n";
        }
    } else {
        CodeGenerator codeGenerator(bfMem_, arguments_);
        auto offset = codeGenerator.compile(prog);
        if (arguments_.verbose) {
            std::cout << "Compiled in " << time() << " seconds\n";
            std::cout << "Used " << codeGenerator.generatedLength() << " bytes\n";
            std::cout << "Running with mem-size: " << arguments_.bfMemLength << " bytes\n";
        }
        if (arguments_.dumpCode) {
            std::cout << "Instructions : " << codeGenerator.instructionHexDump() << '\n';
        }

        time();
        codeGenerator.enter(offset);
        if (arguments_.verbose) {
            std::cout << '\n';
            std::cout << "Executed in " << time() << " seconds\n";
        }
    }
    if (arguments_.dumpMem) {
        std::cout << "Mem: ";
        for (auto i = 0u; i < std::min((size_t)32, bfMem_.size()); ++i) {
            std::cout << (int)bfMem_[i] << ' ';
        }
        std::cout << '\n';
    }
}

template class Engine<char>;
template class Engine<short>;
template class Engine<int>;
