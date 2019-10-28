#include <iostream>
#include <fstream>
#include <stack>
#include "error.hpp"
#include "asmbuf.hpp"
#include <sys/syscall.h>

const char* INS_SYSCALL = "\x0f\x05";
const char* INS_RET  = "\xc3";

const int BFMEM_LENGTH = 3000;

extern "C" unsigned char mgetc() {
    int c = getchar();
    if(c == EOF) {
        std::cerr << "\nError: Unexpected end of input" << std::endl;
        exit(1);
    }
    return c;
}

char bf_mem[BFMEM_LENGTH];

struct BFIns {
    char op;
    size_t length;
private:
    friend std::istream& operator>>(std::istream &is, BFIns &ins);
};

std::istream& operator>>(std::istream &is, BFIns &ins) {
    is >> ins.op;
    ins.length = 1;
    switch(ins.op) {
    case '+':
    case '-':
    case '>':
    case '<':
        while(is.peek() == ins.op) {
            ++ins.length;
            is >> ins.op;
        }
    }
    return is;
}

ASMBuf compile_bf(std::istream &is) {
    BFIns ins;
    ASMBuf rv(10);
    // r10 is bf_mem
    // r11 is the offset into bf_mem
    // r12 is the value of the current cell
    // r13 is putchar
    // r14 is mgetc
    // r15 is BFMEM_LENGTH

    // mov $bf_mem, %r10
    rv.write_bytes({0x49, 0xba});
    rv.write_val((uintptr_t)bf_mem);

    // xor %r11, %r11
    rv.write_bytes({0x4d, 0x31, 0xdb});

    // mov putchar, %r13
    rv.write_bytes({0x49, 0xbd});
    rv.write_val((uintptr_t)putchar);

    // mov mgetc, %r14
    rv.write_bytes({0x49, 0xbe});
    rv.write_val((uintptr_t)mgetc);


    // mov $BFMEM_LENGTH, %r15
    rv.write_bytes({0x49, 0xbf});
    rv.write_val((size_t)BFMEM_LENGTH);

    std::stack<std::pair<uintptr_t,uintptr_t>> loop_starts;
    for(is >> ins; is.good(); is >> ins) {
        switch(ins.op) {
        case '+':
            {
                if(ins.length == 0) continue;
                size_t step = ins.length & 0xff;
                // mov [r10+r11], %r12b
                rv.write_bytes({0x47, 0x8a, 0x24, 0x1a});
                // add $step, %r12b
                rv.write_bytes({0x41, 0x80, 0xc4});
                rv.write_val((unsigned char)step);
                // mov %r12b, [r10+r11]
                rv.write_bytes({0x47, 0x88, 0x24, 0x1a});
            }
            break;
        case '-':
            {
                if(ins.length == 0) continue;
                size_t step = 0x100 - (ins.length & 0xff);
                // mov [r10+r11], %r12b
                rv.write_bytes({0x47, 0x8a, 0x24, 0x1a});
                // add $step, %r12b
                rv.write_bytes({0x41, 0x80, 0xc4});
                rv.write_val((unsigned char)step);
                // mov %r12b, [r10+r11]
                rv.write_bytes({0x47, 0x88, 0x24, 0x1a});
            }
            break;
        case '<':
            {
                if(ins.length == 0) continue;
                size_t step = BFMEM_LENGTH -
                    (ins.length % BFMEM_LENGTH);
                // add $step, %r11
                rv.write_bytes({0x49, 0x81, 0xc3});
                rv.write_val((uint32_t)step);
                // xor %rdx, %rdx
                rv.write_bytes({0x48, 0x31, 0xd2});
                // mov %r11, %rax
                rv.write_bytes({0x4c, 0x89, 0xd8});
                // div %r15
                rv.write_bytes({0x49, 0xf7, 0xf7});
                // mov %rdx, %r11
                rv.write_bytes({0x49, 0x89, 0xd3});
            }
            break;
        case '>':
            {
                if(ins.length == 0) continue;
                size_t step = ins.length % BFMEM_LENGTH;
                if(step == 1) {
                    // inc %r11
                    rv.write_bytes({0x49, 0xff, 0xc3});
                } else {
                    // add $step, %r11
                    rv.write_bytes({0x49, 0x81, 0xc3});
                    rv.write_val((uint32_t)step);
                }
                // xor %rdx, %rdx
                rv.write_bytes({0x48, 0x31, 0xd2});
                // mov %r11, %rax
                rv.write_bytes({0x4c, 0x89, 0xd8});
                // div %r15
                rv.write_bytes({0x49, 0xf7, 0xf7});
                // mov %rdx, %r11
                rv.write_bytes({0x49, 0x89, 0xd3});
            }
            break;
        case '.':
            // push %r10
            rv.write_bytes({0x41, 0x52});
            // push %r11
            rv.write_bytes({0x41, 0x53});
            // push %rbp
            rv.write_bytes({0x55});
            // mov %rsp, %rbp
            rv.write_bytes({0x48, 0x89, 0xe5});
            // mov [r10+r11], %dil
            rv.write_bytes({0x43, 0x8a, 0x3c, 0x1a});
            // call *%r13
            rv.write_bytes({0x41, 0xff, 0xd5});
            // pop %rbp
            rv.write_bytes({0x5d});
            // pop %r11
            rv.write_bytes({0x41, 0x5b});
            // pop %r10
            rv.write_bytes({0x41, 0x5a});
            break;
        case ',':
            // push %r10
            rv.write_bytes({0x41, 0x52});
            // push %r11
            rv.write_bytes({0x41, 0x53});
            // push %rbp
            rv.write_bytes({0x55});
            // mov %rsp, %rbp
            rv.write_bytes({0x48, 0x89, 0xe5});
            // call *%r14
            rv.write_bytes({0x41, 0xff, 0xd6});
            // pop %rbp
            rv.write_bytes({0x5d});
            // pop %r11
            rv.write_bytes({0x41, 0x5b});
            // pop %r10
            rv.write_bytes({0x41, 0x5a});
            // mov %al, [r10+r11]
            rv.write_bytes({0x43, 0x88, 0x04, 0x1a});
            break;
        case '[':
            {
                // mark start of loop
                uintptr_t loop_start = rv.current_offset();
                // mov [r10+r11], r12b
                rv.write_bytes({0x47, 0x8a, 0x24, 0x1a});
                // test r12b, r12b
                rv.write_bytes({0x45, 0x84, 0xe4});
                // store patch_loc
                uintptr_t patch_loc = rv.current_offset();
                // jz 0
                rv.write_bytes({0x0f, 0x84, 0x00, 0x00, 0x00, 0x00});
                // add loop start info to loop_starts
                loop_starts.emplace(loop_start, patch_loc);
            }
            break;
        case ']':
            {
                if(loop_starts.size() < 1) {
                    throw JITError("Unmatched ]");
                }
                auto stack_top = loop_starts.top(); loop_starts.pop();
                uintptr_t loop_start = stack_top.first;
                uintptr_t patch_loc = stack_top.second;
                // jmpq diff
                {
                    uintptr_t current_pos = rv.current_offset();
                    int32_t jump_instruction_length = 5;
                    int32_t rel_off = (int32_t)loop_start
                        - (int32_t)current_pos
                        - jump_instruction_length;
                    rv.write_bytes({0xe9});
                    rv.write_val(rel_off);
                }
                // patch start of jump
                {
                    uintptr_t current_pos = rv.current_offset();
                    int32_t forward_jump_instruction_length = 6;
                    int32_t forward_off = (int32_t)current_pos
                        - (int32_t)patch_loc
                        - forward_jump_instruction_length;
                    size_t forward_jump_opcode_length = 2;
                    size_t patch_offset_loc = patch_loc + forward_jump_opcode_length;
                    rv.patch_val(patch_offset_loc, forward_off);
                }
            }
            break;
        }
    }
    if(loop_starts.size() > 0) {
        throw JITError("Unmatched [");
    }
    rv.write_str(INS_RET);
    return std::move(rv);
}

int main(int argc, const char *argv[]) {
    if(argc < 2) {
        std::cerr << "Usage: " << argv[0] << " infile\n";
        return 1;
    }
    auto in = std::ifstream(argv[1]);
    auto ab = std::move(compile_bf(in));
    //std::cout << "ASM Length: " << ab.current_offset() << "\nGenerated ASM: ";
    //ab.print_buf_instructions();
    ab.set_executable(1);
    enter_buf(ab.get_offset(0));
    return 0;
}
