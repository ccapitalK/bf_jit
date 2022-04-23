#include <iostream>

#include "elf_parser.hpp"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "usage: " << argv[0] << "object_file output_file_base\n";
        return 0;
    }
    ElfParser parser(argv[1]);
    std::cout << "Name: " << parser.getFilename() << '\n';
    std::cout << "Kind: " << parser.getElfKind() << '\n';
    return 0;
}