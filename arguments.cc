#include <cxxopts.hpp>

#include "arguments.hpp"

Arguments::Arguments(int argc, char *argv[]) {
    cxxopts::Options options("bf_jit", "JIT-compiling interpeter for brainfuck");
    options.add_options()
        .operator()("m,mem-size", "Number of memory cells", cxxopts::value<size_t>()->default_value("32768"))
        .operator()("file-names", "BF source file names", cxxopts::value<std::vector<std::string>>())("d,dump-code", "Dump the generated machine code")
        .operator()("g,gen-syms", "Generate jit symbol maps for debugging purposes")
        .operator()("h,help", "Print help")
        .operator()("v,verbose", "Print more information");
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
