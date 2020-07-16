#include <cxxopts.hpp>

#include "arguments.hpp"

Arguments::Arguments(int argc, char *argv[]) {
    cxxopts::Options options("bf_jit", "JIT-compiling interpeter for brainfuck");
    const auto eofBehavioursParseType = cxxopts::value<std::string>()->default_value("return-0");
    const auto eofBehaviourOptionDescription = "Behaviour on eof (one of return-0, return-255, dont-modify)";
    // clang-format off
    options.add_options()
        .operator()("m,mem-size", "Number of memory cells", cxxopts::value<size_t>()->default_value("32768"))
        .operator()("file-names", "BF source file names", cxxopts::value<std::vector<std::string>>())
        .operator()("d,dump-code", "Dump the generated machine code")
        .operator()("e,eof-behaviour", eofBehaviourOptionDescription, eofBehavioursParseType)
        .operator()("g,gen-syms", "Generate jit symbol maps for debugging purposes")
        .operator()("h,help", "Print help")
        .operator()("n,no-flush", "Don't flush after each character")
        .operator()("w,cell-bit-width", "Width of cell in bits", cxxopts::value<size_t>()->default_value("8"))
        .operator()("use-interpreter", "Don't jit the IR, just interpret it")
        .operator()("v,verbose", "Print more information");
    // clang-format on
    options.positional_help("[input files]").show_positional_help();
    options.parse_positional({"file-names"});
    try {
        auto result = options.parse(argc, argv);
        if (result.count("help")) {
            std::cout << options.help() << '\n';
            exit(0);
        }
        verbose = result.count("verbose");
        genSyms = result.count("gen-syms");
        dumpCode = result.count("dump-code");
        noFlush = result.count("no-flush");
        useInterpreter = result.count("use-interpreter");
        bfMemLength = result["mem-size"].as<size_t>();
        cellBitWidth = result["cell-bit-width"].as<size_t>();
        if (cellBitWidth != 8 && cellBitWidth != 16 && cellBitWidth != 32) {
            throw cxxopts::OptionException("Invalid cell width");
        }
        if (result.count("eof-behaviour")) {
            const auto behaviour = result["eof-behaviour"].as<std::string>();
            if (behaviour == "return-0") {
                getCharBehaviour = GetCharBehaviour::EOF_RETURNS_0;
            } else if (behaviour == "return-255") {
                getCharBehaviour = GetCharBehaviour::EOF_RETURNS_255;
            } else if (behaviour == "dont-modify") {
                getCharBehaviour = GetCharBehaviour::EOF_DOESNT_MODIFY;
            } else {
                throw cxxopts::OptionException("Invalid argument for eof-behaviour");
            }
        }
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
