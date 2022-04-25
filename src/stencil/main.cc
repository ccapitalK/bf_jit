#include <iostream>
#include <string>

#include "elf_parser.hpp"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "usage: " << argv[0] << "object_file output_file_base\n";
        return 0;
    }
    ElfParser parser(argv[1]);
    std::cout << "Name: " << parser.getFilename() << '\n';
    std::string kind = parser.getElfKind();
    std::cout << "Kind: " << kind << '\n';
    if (kind != "elf object") {
        std::cout << "Not an elf object, exiting\n";
        return 1;
    }
    std::cout << "Number of functions: " << parser.countFunctions() << '\n';
    return 0;
}