#include <ctime>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <stack>
#include "error.hpp"
#include "asmbuf.hpp"
#include "parse.hpp"
//#include <sys/syscall.h>

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

extern "C" int mputchar(int c) {
    fflush(stdout);
    return putchar(c);
}

char bf_mem[BFMEM_LENGTH];

ASMBuf compile_bf(std::vector<Instruction> &prog) {
    ASMBuf rv(1);
    // Register model:
    // r10 = &bf_mem[0]
    // r11 is the offset into bf_mem
    // r12 is the value of the current cell
    // r13 is the address of mputchar
    // r14 is the address of mgetc
    // r15 is BFMEM_LENGTH

    // Prelude to initialize registers as listed above
    rv.write_bytes({
    // mov $bf_mem, %r10
        0x49, 0xba
    });
    rv.write_val((uintptr_t)bf_mem);

    rv.write_bytes({
    // xor %r11, %r11
        0x4d, 0x31, 0xdb,
    // mov putchar, %r13
        0x49, 0xbd
    });
    rv.write_val((uintptr_t)mputchar);

    rv.write_bytes({
    // mov mgetc, %r14
        0x49, 0xbe
    });
    rv.write_val((uintptr_t)mgetc);

    rv.write_bytes({
    // mov $BFMEM_LENGTH, %r15
        0x49, 0xbf
    });
    rv.write_val((size_t)BFMEM_LENGTH);

    std::unordered_map<size_t, std::pair<uintptr_t,uintptr_t>> loopStarts;
    for (auto ins: prog) {
        std::cout << ins << '\n';
        switch(ins.code_) {
        case IROpCode::INS_ADD:
            {
                size_t step;
                // TODO: Fix
                if (ins.a_ > 0) {
                    step = ins.a_ & 0xff;
                } else {
                    step = 0x100 - ((-ins.a_) & 0xff);
                }
                rv.write_bytes({
                // mov [r10+r11], %r12b
                    0x47, 0x8a, 0x24, 0x1a,
                // add $step, %r12b
                    0x41, 0x80, 0xc4
                });
                rv.write_val((unsigned char)step);
                rv.write_bytes({
                // mov %r12b, [r10+r11]
                    0x47, 0x88, 0x24, 0x1a
                });
            }
            break;
        case IROpCode::INS_ZERO:
            {
                rv.write_bytes({
                // xor %r12, %r12
                    0x4d, 0x31, 0xe4,
                // mov %r12b, [r10+r11]
                    0x47, 0x88, 0x24, 0x1a
                });
            }
            break;
        case IROpCode::INS_ADP:
            {
                size_t step;
                // TODO: Fix
                if (ins.a_ < 0) {
                    step = BFMEM_LENGTH -
                        ((-ins.a_) % BFMEM_LENGTH);
                } else {
                    step = ins.a_ % BFMEM_LENGTH;
                }
                if(step == 1) {
                    // inc %r11
                    rv.write_bytes({0x49, 0xff, 0xc3});
                } else {
                    rv.write_bytes({
                    // add $step, %r11
                        0x49, 0x81, 0xc3
                    });
                    rv.write_val((uint32_t)step);
                }
                rv.write_bytes({
                // xor %rdx, %rdx
                    0x48, 0x31, 0xd2,
                // mov %r11, %rax
                    0x4c, 0x89, 0xd8,
                // div %r15
                    0x49, 0xf7, 0xf7,
                // mov %rdx, %r11
                    0x49, 0x89, 0xd3
                });
            }
            break;
        case IROpCode::INS_OUT:
            rv.write_bytes({
            // push %r10
                0x41, 0x52,
            // push %r11
                0x41, 0x53,
            // push %rbp
                0x55,
            // mov %rsp, %rbp
                0x48, 0x89, 0xe5,
            // mov [r10+r11], %dil
                0x43, 0x8a, 0x3c, 0x1a,
            // call *%r13
                0x41, 0xff, 0xd5,
            // pop %rbp
                0x5d,
            // pop %r11
                0x41, 0x5b,
            // pop %r10
                0x41, 0x5a
            });
            break;
        case IROpCode::INS_IN:
            rv.write_bytes({
            // push %r10
                0x41, 0x52,
            // push %r11
                0x41, 0x53,
            // push %rbp
                0x55,
            // mov %rsp, %rbp
                0x48, 0x89, 0xe5,
            // call *%r14
                0x41, 0xff, 0xd6,
            // pop %rbp
                0x5d,
            // pop %r11
                0x41, 0x5b,
            // pop %r10
                0x41, 0x5a,
            // mov %al, [r10+r11]
                0x43, 0x88, 0x04, 0x1a
            });
            break;
        case IROpCode::INS_LOOP:
            {
                // mark start of loop
                uintptr_t loop_start = rv.current_offset();
                // mov [r10+r11], r12b
                rv.write_bytes({
                    0x47, 0x8a, 0x24, 0x1a,
                // test r12b, r12b
                    0x45, 0x84, 0xe4
                });
                // store patch_loc
                uintptr_t patch_loc = rv.current_offset();
                rv.write_bytes({
                    // jz 0
                    0x0f, 0x84, 0x00, 0x00, 0x00, 0x00
                });
                // add loop start info to loopStarts
                loopStarts.emplace(ins.a_, std::make_pair(loop_start, patch_loc));
            }
            break;
        case IROpCode::INS_END:
            {
                auto loopInfo = loopStarts[ins.a_];
                uintptr_t loop_start = loopInfo.first;
                uintptr_t patch_loc = loopInfo.second;
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
        default:
            break;
        }
    }
    rv.write_str(INS_RET);
    return rv;
}

double time() {
    static std::clock_t start_time = std::clock();
    std::clock_t now = std::clock();
    double duration = (now-start_time) / (double) CLOCKS_PER_SEC;
    start_time = now;
    return duration;
}

constexpr size_t RDBUF_SIZE = 256*1024;
static char rdbuf[RDBUF_SIZE];

int main(int argc, const char *argv[]) {
    if(argc < 2) {
        std::cerr << "Usage: " << argv[0] << " infile\n";
        return 1;
    }
    try {
        auto in = std::ifstream(argv[1]);
        in.rdbuf()->pubsetbuf(rdbuf, RDBUF_SIZE);
        time();
        Parser parser;
        parser.feed(in);
        auto prog = parser.compile();
        auto ab {compile_bf(prog)};
        std::cout << "Compiled in " << time() << " seconds\n";
        std::cout << "Used " << ab.length() << " bytes\n";
        std::cout << "ASM Length: " << ab.current_offset() << "\n";
        // std::cout << "Generated ASM: ";
        // ab.print_buf_instructions();

        time();
        ab.set_executable(true);
        enter_buf(ab.get_offset(0));
        std::cout << "Executed in " << time() << " seconds\n";
        return 0;
    } catch (JITError &e) {
        std::cout << "Fatal JITError caught: " << e.what() << '\n';
        return 1;
    }
}
