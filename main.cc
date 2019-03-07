#include <iostream>
#include <fstream>
#include "error.hpp"
#include "asmbuf.hpp"
#include <sys/syscall.h>

const char* INS_SYSCALL = "\x0f\x05";
const char* INS_RET  = "\xc3";

extern "C" unsigned char mgetc() {
    int c = getc(stdin);
    if(c == EOF) {
        std::cerr << "\nUnexpected end of input" << std::flush;
        exit(1);
    }
    return c;
}

ASMBuf compile_bf(std::istream &is) {
    char c;
    ASMBuf rv(10);
    for(is >> c; is.good(); is >> c) {
        switch(c) {
        case '+':
        case '-':
        case '<':
        case '>':
        case '.':
        case ',':
        case '[':
        case ']':
            break;
        }
    }
    rv.write_str(INS_RET);
    return std::move(rv);
}

void jit_exit(char code) {
    ASMBuf ab(10);
    ab.set_executable(0);
    // mov eax, 60
    ab.write_bytes({0x48, 0xc7, 0xc0});
    ab.write_val((unsigned int)SYS_exit);
    // mov di, 5
    ab.write_bytes({0x66, 0xbf});
    ab.write_val((unsigned short)code);
    // syscall
    ab.write_str(INS_SYSCALL);
    // ret
    ab.write_str(INS_RET);

    ab.set_executable(1);
    enter_buf(ab.get_offset(0));
}

int main(int argc, const char *argv[]) {
    if(argc < 2) {
        std::cerr << "Usage: " << argv[0] << " infile\n";
        return 1;
    }
    jit_exit(10);
    //auto in = std::ifstream(argv[1]);
    //auto ab = std::move(compile_bf(in));
    //ab.print_buf_instructions();
    //enter_buf(ab.get_offset(0));
    return 0;
}
