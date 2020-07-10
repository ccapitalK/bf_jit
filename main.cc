#include "asmbuf.hpp"
#include "error.hpp"
#include "parse.hpp"
#include <ctime>
#include <fstream>
#include <iostream>
#include <stack>
#include <unordered_map>

const char *INS_SYSCALL = "\x0f\x05";
const char *INS_RET = "\xc3";

// If this isn't signed, calculations in the code generator default to unsigned, and would require a lot of casting
const ssize_t BFMEM_LENGTH = 4096;

extern "C" unsigned char mgetc() {
    int c = getchar();
    if (c == EOF) {
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

template <typename T> bool is_pow_2(T v) {
    size_t nonZeroBits = 0;
    for (; v != 0; v >>= 1) {
        nonZeroBits += v & 1;
    }
    return nonZeroBits == 1;
}

void interpret(const std::vector<Instruction> &prog) {
    if (!is_pow_2(BFMEM_LENGTH)) {
        throw JITError("Need pow 2 arena");
    }
    size_t dp{};

    std::vector<std::pair<uintptr_t, uintptr_t>> loopPositions;
    for (size_t i = 0; i < prog.size(); ++i) {
        const auto &ins = prog[i];
        switch (ins.code_) {
        case IROpCode::INS_LOOP:
            if (ins.a_ >= (ssize_t)loopPositions.size()) {
                loopPositions.resize(ins.a_ + 1);
            }
            loopPositions[ins.a_].first = i;
            break;
        case IROpCode::INS_END:
            if (ins.a_ >= (ssize_t)loopPositions.size()) {
                loopPositions.resize(ins.a_ + 1);
            }
            loopPositions[ins.a_].second = i;
            break;
        default:
            break;
        }
    }
    const auto mask = BFMEM_LENGTH - 1;
    for (size_t i = 0; i < prog.size(); ++i) {
        auto &ins = prog[i];
        switch (ins.code_) {
        case IROpCode::INS_ADD:
            bf_mem[dp] += ins.a_;
            break;
        case IROpCode::INS_MUL: {
            auto remote = (dp + ins.a_) & mask;
            bf_mem[remote] += ins.b_ * bf_mem[dp];
        } break;
        case IROpCode::INS_ZERO:
            bf_mem[dp] = 0;
            break;
        case IROpCode::INS_ADP:
            dp = (dp + ins.a_) & mask;
            break;
        case IROpCode::INS_IN:
            bf_mem[dp] = mgetc();
            break;
        case IROpCode::INS_OUT:
            mputchar(bf_mem[dp]);
            break;
        case IROpCode::INS_LOOP:
            if (bf_mem[dp] == 0) {
                i = loopPositions[ins.a_].second;
            }
            break;
        case IROpCode::INS_END:
            i = loopPositions[ins.a_].first - 1;
            break;
        default:
            throw JITError("ICE: Unhandled instruction");
        }
    }
}

ASMBuf compile_bf(const std::vector<Instruction> &prog) {
    ASMBuf rv(1);
    bool isPow2MemLength = is_pow_2(BFMEM_LENGTH);
    // Instructions in these comments use "$op $src, $dest" convention
    //
    // Register model:
    // r10 = &bf_mem[0]
    // r11 is the offset into bf_mem
    // r12 is sometimes used to store the value of the current cell
    // r13 is the address of mputchar
    // r14 is the address of mgetc
    // r15 is (BFMEM_LENGTH-1) if isPow2MemLength, else it is BFMEM_LENGTH

    /// Prelude to save callee-saved registers
    rv.write_bytes({
    // push %r12
        0x41, 0x54,
    // push %r13
        0x41, 0x55,
    // push %r14
        0x41, 0x56,
    // push %r15
        0x41, 0x57
    });
    /// Prelude to initialize registers as listed above
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
    rv.write_val((size_t)BFMEM_LENGTH - (size_t)isPow2MemLength);

    std::unordered_map<size_t, std::pair<uintptr_t, uintptr_t>> loopStarts;
    for (auto ins : prog) {
        // std::cout << "Instruction: " << ins << '\n';
        switch (ins.code_) {
        case IROpCode::INS_ADD: {
            size_t step = ins.a_ & 0xff;
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
        } break;
        case IROpCode::INS_ZERO: {
            rv.write_bytes({
            // xor %r12, %r12
                0x4d, 0x31, 0xe4,
            // mov %r12b, [r10+r11]
                0x47, 0x88, 0x24, 0x1a
            });
        } break;
        case IROpCode::INS_MUL: {
            // map ins.a_ into [0, BFMEM_LENGTH), needed because of C++'s % weirdness with negative numbers
            const size_t destOffset = ((ins.a_ % BFMEM_LENGTH) + BFMEM_LENGTH) % BFMEM_LENGTH;
            const char multFactor = (char)ins.b_;
            /// Strategy:
            /// load offset of remote into rax
            rv.write_bytes({
            // mov %r11d, %eax
                0x44, 0x89, 0xd8,
            // add $destOffset, %eax
                0x05
            });
            rv.write_val((uint32_t)destOffset);
            if (isPow2MemLength) {
                rv.write_bytes({
                // and %r15d, %eax
                    0x44, 0x21, 0xf8
                });
            } else {
                /// eax -= r15d * (eax >= r15d);
                /// This works because at this point, we know that 0 <= r11d < 2*r15d - 1
                rv.write_bytes({
                // xor %edx, %edx
                    0x31, 0xd2,
                // cmp %r15d, %eax
                    0x44, 0x39, 0xf8,
                // cmovge %edx, %r15d
                    0x41, 0x0f, 0x4d, 0xd7,
                // sub %edx, %eax
                    0x29, 0xd0
                });
            }
            rv.write_bytes({
            // mov %rax, %rdx
                0x48, 0x89, 0xc2,
            /// load current cell into r12
            // mov [r10+r11], %r12b
                0x47, 0x8a, 0x24, 0x1a,
            /// mult r12 by multFactor, store in al
            // mov multFactor, %al
                0xb0, (unsigned char)multFactor,
            // mul r12b
                0x41, 0xf6, 0xe4,
            /// load value at remote
            // mov [r10+rdx], %r12b
                0x45, 0x8a, 0x24, 0x12,
            /// add together
            // add %r12b, %al
                0x44, 0x00, 0xe0,
            /// store at remote
            // mov %al, [r10+rdx]
                0x41, 0x88, 0x04, 0x12,
            });
        } break;
        case IROpCode::INS_ADP: {
            // map ins.a_ into [0, BFMEM_LENGTH), needed because of C++'s % weirdness with negative numbers
            const int step = ((ins.a_ % BFMEM_LENGTH) + BFMEM_LENGTH) % BFMEM_LENGTH;
            if (step == 1) {
                rv.write_bytes({
                // inc %r11
                    0x49, 0xff, 0xc3
                });
            } else {
                rv.write_bytes({
                // add $step, %r11
                    0x49, 0x81, 0xc3
                });
                rv.write_val((uint32_t)step);
            }
            if (isPow2MemLength) {
                rv.write_bytes({
                // and %r15d, r11d
                    0x45, 0x21, 0xfb
                });
            } else {
                /// r11d -= r15d * (r11d >= r15d);
                /// This works because at this point, we know that 0 <= r11d < 2*r15d - 1
                rv.write_bytes({
                // xor %eax, %eax
                    0x31, 0xc0,
                // cmp %r15d, %r11d
                    0x45, 0x39, 0xfb,
                // cmovge %r15d, %eax
                    0x41, 0x0f, 0x4d, 0xc7,
                // sub %eax, %r11d
                    0x41, 0x29, 0xc3,
                });
            }
        } break;
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
        case IROpCode::INS_LOOP: {
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
        } break;
        case IROpCode::INS_END: {
            const auto loopInfo = loopStarts[ins.a_];
            const uintptr_t loop_start = loopInfo.first;
            const uintptr_t patch_loc = loopInfo.second;
            {
                const uintptr_t current_pos = rv.current_offset();
                const int32_t jump_instruction_length = 5;
                const int32_t rel_off = (int32_t)loop_start - (int32_t)current_pos - jump_instruction_length;
                rv.write_bytes({
                // jmpq diff
                    0xe9
                });
                rv.write_val(rel_off);
            }
            // patch start of jump
            {
                const uintptr_t current_pos = rv.current_offset();
                const int32_t forward_jump_instruction_length = 6;
                const int32_t forward_off = (int32_t)current_pos - (int32_t)patch_loc - forward_jump_instruction_length;
                const size_t forward_jump_opcode_length = 2;
                const size_t patch_offset_loc = patch_loc + forward_jump_opcode_length;
                rv.patch_val(patch_offset_loc, forward_off);
            }
        } break;
        default:
            throw JITError("ICE: Unhandled instruction");
        }
    }
    /// Restore callee-saved registers
    rv.write_bytes({
    // pop %r15
        0x41, 0x5f,
    // pop %r14
        0x41, 0x5e,
    // pop %r13
        0x41, 0x5d,
    // pop %r12
        0x41, 0x5c
    });
    rv.write_str(INS_RET);
    return rv;
}

double time() {
    static std::clock_t start_time = std::clock();
    std::clock_t now = std::clock();
    double duration = (now - start_time) / (double)CLOCKS_PER_SEC;
    start_time = now;
    return duration;
}

constexpr size_t RDBUF_SIZE = 256 * 1024;
static char rdbuf[RDBUF_SIZE];

int main(int argc, const char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " infile\n";
        return 1;
    }
    try {
        auto in = std::ifstream(argv[1]);
        if (!in.good()) {
            throw JITError("Failed to open file");
        }
        in.rdbuf()->pubsetbuf(rdbuf, RDBUF_SIZE);
        time();
        Parser parser;
        parser.feed(in);
        auto prog = parser.compile();
        auto ab{compile_bf(prog)};
        std::cout << "Compiled in " << time() << " seconds\n";
        std::cout << "Used " << ab.length() << " bytes\n";
        std::cout << "ASM Length: " << ab.current_offset() << "\n";
        // std::cout << "Generated ASM: ";
        // ab.print_buf_instructions();

        time();
        ab.set_executable(true);
        enter_buf(ab.get_offset(0));
        std::cout << '\n';
        std::cout << "Executed in " << time() << " seconds\n";
        return 0;
    } catch (JITError &e) {
        std::cout << "Fatal JITError caught: " << e.what() << '\n';
        return 1;
    }
}
