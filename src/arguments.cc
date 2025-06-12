#include <iostream>
#include <getopt.h>
#include <cstdlib>
#include <cstring>
#include "arguments.hpp"

// Ported from cxxopt to getopt by claude

void Arguments::printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " [OPTIONS] [input files]\n\n"
              << "JIT-compiling interpreter for brainfuck\n\n"
              << "Options:\n"
              << "  -m, --mem-size SIZE        Number of memory cells (default: 32768)\n"
              << "  -w, --cell-bit-width BITS  Width of cell in bits (8, 16, or 32, default: 8)\n"
              << "  -d, --dump-code            Dump the generated machine code\n"
              << "      --dry-run              Compile the code, but don't run it\n"
              << "      --dump-mem             Dump the first 32 cells of memory after termination\n"
              << "  -e, --eof-behaviour MODE   Behaviour on eof (return-0, return-255, dont-modify, default: return-0)\n"
              << "  -g, --gen-syms             Generate jit symbol maps for debugging purposes\n"
              << "  -n, --no-flush             Don't flush after each character\n"
              << "      --use-interpreter      Don't jit the IR, just interpret it\n"
              << "  -v, --verbose              Print more information\n"
              << "  -h, --help                 Print this help message\n";
}

Arguments::Arguments(int argc, char *argv[]) :
    bfMemLength(32768),
    cellBitWidth(8),
    getCharBehaviour(GetCharBehaviour::EOF_RETURNS_0) {

    static struct option long_options[] = {
        {"mem-size", required_argument, 0, 'm'},
        {"cell-bit-width", required_argument, 0, 'w'},
        {"dump-code", no_argument, 0, 'd'},
        {"dry-run", no_argument, 0, 1001},
        {"dump-mem", no_argument, 0, 1002},
        {"eof-behaviour", required_argument, 0, 'e'},
        {"gen-syms", no_argument, 0, 'g'},
        {"no-flush", no_argument, 0, 'n'},
        {"use-interpreter", no_argument, 0, 1003},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c;

    while ((c = getopt_long(argc, argv, "m:w:de:gnvh", long_options, &option_index)) != -1) {
        switch (c) {
            case 'm':
                bfMemLength = std::strtoul(optarg, nullptr, 10);
                break;
            case 'w':
                cellBitWidth = std::strtoul(optarg, nullptr, 10);
                if (cellBitWidth != 8 && cellBitWidth != 16 && cellBitWidth != 32) {
                    std::cerr << "Error: Invalid cell width. Must be 8, 16, or 32.\n";
                    printUsage(argv[0]);
                    exit(1);
                }
                break;
            case 'd':
                dumpCode = true;
                break;
            case 1001: // --dry-run
                dryRun = true;
                break;
            case 1002: // --dump-mem
                dumpMem = true;
                break;
            case 'e':
                if (strcmp(optarg, "return-0") == 0) {
                    getCharBehaviour = GetCharBehaviour::EOF_RETURNS_0;
                } else if (strcmp(optarg, "return-255") == 0) {
                    getCharBehaviour = GetCharBehaviour::EOF_RETURNS_255;
                } else if (strcmp(optarg, "dont-modify") == 0) {
                    getCharBehaviour = GetCharBehaviour::EOF_DOESNT_MODIFY;
                } else {
                    std::cerr << "Error: Invalid argument for eof-behaviour. Must be one of: return-0, return-255, dont-modify\n";
                    printUsage(argv[0]);
                    exit(1);
                }
                break;
            case 'g':
                genSyms = true;
                break;
            case 'n':
                noFlush = true;
                break;
            case 1003: // --use-interpreter
                useInterpreter = true;
                break;
            case 'v':
                verbose = true;
                break;
            case 'h':
                printUsage(argv[0]);
                exit(0);
                break;
            case '?':
                // getopt_long already printed an error message
                printUsage(argv[0]);
                exit(1);
                break;
            default:
                std::cerr << "Error: Unknown option\n";
                printUsage(argv[0]);
                exit(1);
        }
    }

    // Collect remaining arguments as file names
    for (int i = optind; i < argc; i++) {
        fileNames.push_back(argv[i]);
    }

    if (fileNames.empty()) {
        std::cerr << "Error: No source files specified\n";
        printUsage(argv[0]);
        exit(1);
    }
}
