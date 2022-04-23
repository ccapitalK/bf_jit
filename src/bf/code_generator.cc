#include <iostream>
#include <optional>
#include <stack>
#include <tuple>
#include <vector>

#include "code_generator.hpp"
#include "runtime.hpp"

// Instructions in these comments use intel syntax
//
// Register model:
// r10 = &bfMem[0]
// r11 is the index into bfMem_
// r12 is sometimes used to store the value of the current cell
// r13 is the address of mputchar
// r14 is the address of mgetc
// r15 is (BFMEM_LENGTH-1) if IS_POW_2_MEM_LENGTH, else it is BFMEM_LENGTH

template <typename CellType>
ASMBufOffset CodeGenerator<CellType>::compile(const std::vector<Instruction> &prog) {
    std::vector<std::pair<ASMBufOffset, Instruction>> symbolMap;
    buf_.set_executable(false);
    const auto startOffset = buf_.current_offset();
    symbolMap.emplace_back(startOffset, Instruction{});
    generatePrelude();
    for (auto ins : prog) {
        // std::cout << "Instruction: " << ins << '\n';
        if (genPerfMap_) {
            symbolMap.emplace_back(buf_.current_offset(), ins);
        }
        switch (ins.code_) {
        case IROpCode::ADD:
            generateInsAdd(ins.a_);
            break;
        case IROpCode::ADP:
            generateInsAdp(ins.a_);
            break;
        case IROpCode::MUL:
            generateInsMul(ins.a_, ins.b_);
            break;
        case IROpCode::CONST:
            generateInsConst(ins.a_);
            break;
        case IROpCode::OUT:
            generateInsOut();
            break;
        case IROpCode::IN:
            generateInsIn();
            break;
        case IROpCode::LOOP:
            generateInsLoop(ins.a_);
            break;
        case IROpCode::END_LOOP:
            generateInsEndLoop(ins.a_);
            break;
        default:
            throw JITError("ICE: Unhandled instruction");
        }
    }
    symbolMap.emplace_back(buf_.current_offset(), Instruction{});
    generateEpilogue();
    symbolMap.emplace_back(buf_.current_offset(), Instruction{});
    if (genPerfMap_) {
        // FIXME: make this work across multiple calls to compile()
        for (auto i = 0u; i + 1 < symbolMap.size(); ++i) {
            const ASMBufOffset start = symbolMap[i].first;
            const ASMBufOffset end = symbolMap[i+1].first;
            const size_t size = end - start;
            perfSymbolMap_
                << std::hex << buf_.address_at_offset(start) << ' '
                << size << ' ' << std::dec;
            if (i == 0) {
                perfSymbolMap_ << "jit_prelude\n";
            } else if (i + 2 == symbolMap.size()) {
                perfSymbolMap_ << "jit_epilogue\n";
            } else {
                perfSymbolMap_ << "JIT OP: #" << i << ' ' << symbolMap[i].second << '\n';
            }
        }
    }
    return startOffset;
}

template <typename CellType>
void CodeGenerator<CellType>::generatePrelude() {
    /// Prelude to save callee-saved registers
    buf_.write_bytes({
    // push %r12
        0x41, 0x54,
    // push %r13
        0x41, 0x55,
    // push %r14
        0x41, 0x56,
    // push %r15
        0x41, 0x57
    });
    /// Prelude to initialize registers as per model
    buf_.write_bytes({
    // mov %r10, $bfMem_
        0x49, 0xba
    });
    buf_.write_val((uintptr_t)bfMem_.data());

    buf_.write_bytes({
    // xor %r11, %r11
        0x4d, 0x31, 0xdb,
    // mov %r13, putchar
        0x49, 0xbd
    });
    buf_.write_val((uintptr_t)putChar_);

    buf_.write_bytes({
    // mov %r14, mgetc
        0x49, 0xbe
    });
    buf_.write_val((uintptr_t)getChar_);

    buf_.write_bytes({
    // mov %r15, $BFMEM_LENGTH
        0x49, 0xbf
    });
    buf_.write_val((size_t)BFMEM_LENGTH - (size_t)IS_POW_2_MEM_LENGTH);
}

template <typename CellType>
void CodeGenerator<CellType>::generateInsAdd(CellType step) {
    if constexpr (std::is_same<CellType, char>::value) {
        buf_.write_bytes({
        // mov %r12b, [r10+r11]
            0x47, 0x8a, 0x24, 0x1a,
        // add %r12b, $step
            0x41, 0x80, 0xc4, (unsigned char) step,
        // mov [r10+r11], %r12b
            0x47, 0x88, 0x24, 0x1a
        });
    } else if constexpr (std::is_same<CellType, short>::value) {
        buf_.write_bytes({
        // mov %r12w, [r10+r11*2]
            0x66, 0x47, 0x8b, 0x24, 0x5a,
        // add %r12w, $step
            0x66, 0x41, 0x81, 0xc4
        });
        buf_.write_val(step);
        buf_.write_bytes({
        // mov [r10+r11*2], %r12w
            0x66, 0x47, 0x89, 0x24, 0x5a
        });
    } else {
        buf_.write_bytes({
        // mov %r12d, [r10+r11*4]
            0x47, 0x8b, 0x24, 0x9a,
        // add %r12d, $step
            0x41, 0x81, 0xc4
        });
        buf_.write_val(step);
        buf_.write_bytes({
        // mov [r10+r11], %r12b
            0x47, 0x89, 0x24, 0x9a
        });
    }
}

template <typename CellType>
void CodeGenerator<CellType>::generateInsAdp(int step) {
    // map ins.a_ into [0, BFMEM_LENGTH), needed because of C++'s % weirdness with negative numbers
    const int adjustedStep = ((step % BFMEM_LENGTH) + BFMEM_LENGTH) % BFMEM_LENGTH;
    if (adjustedStep == 1) {
        buf_.write_bytes({
        // inc %r11
            0x49, 0xff, 0xc3
        });
    } else {
        buf_.write_bytes({
        // add %r11, $adjustedStep
            0x49, 0x81, 0xc3
        });
        buf_.write_val((uint32_t)adjustedStep);
    }
    if (IS_POW_2_MEM_LENGTH) {
        buf_.write_bytes({
        // and %r11d, r15d
            0x45, 0x21, 0xfb
        });
    } else {
        /// r11d -= r15d * (r11d >= r15d);
        /// This works because at this point, we know that 0 <= r11d < 2*r15d - 1
        buf_.write_bytes({
        // xor %eax, %eax
            0x31, 0xc0,
        // cmp %r11d, %r15d
            0x45, 0x39, 0xfb,
        // cmovge %eax, %r15d
            0x41, 0x0f, 0x4d, 0xc7,
        // sub %r11d, %eax
            0x41, 0x29, 0xc3,
        });
    }
}

template <typename CellType>
void CodeGenerator<CellType>::generateInsEndLoop(int loopNumber) {
    const auto loopInfo = loopStarts_[loopNumber];
    const uintptr_t loop_start = loopInfo.first;
    const uintptr_t patch_loc = loopInfo.second;
    {
        const uintptr_t current_pos = buf_.current_offset();
        const int32_t jump_instruction_length = 5;
        const int32_t rel_off = (int32_t)loop_start - (int32_t)current_pos - jump_instruction_length;
        buf_.write_bytes({
        // jmpq $diff
            0xe9
        });
        buf_.write_val(rel_off);
    }
    // patch start of jump
    {
        const uintptr_t current_pos = buf_.current_offset();
        const int32_t forward_jump_instruction_length = 6;
        const int32_t forward_off = (int32_t)current_pos - (int32_t)patch_loc - forward_jump_instruction_length;
        const size_t forward_jump_opcode_length = 2;
        const size_t patch_offset_loc = patch_loc + forward_jump_opcode_length;
        buf_.patch_val(patch_offset_loc, forward_off);
    }
}


template <typename CellType>
void CodeGenerator<CellType>::generateInsIn() {
    if (getCharBehaviour == GetCharBehaviour::EOF_DOESNT_MODIFY) {
        buf_.write_bytes({
        // xor %edi, %edi
            0x31, 0xff,
        });
        if constexpr (std::is_same<CellType, char>::value) {
            buf_.write_bytes({
            // mov %dil, [r10+r11]
                0x43, 0x8a, 0x3c, 0x1a,
            });
        } else if constexpr (std::is_same<CellType, short>::value) {
            buf_.write_bytes({
            // mov %dil, [r10+r11*2]
                0x43, 0x8a, 0x3c, 0x5a,
            });
        } else {
            buf_.write_bytes({
            // mov %dil, [r10+r11*4]
                0x43, 0x8a, 0x3c, 0x9a,
            });
        }
    }
    buf_.write_bytes({
    // push %r10
        0x41, 0x52,
    // push %r11
        0x41, 0x53,
    // push %rbp
        0x55,
    // mov %rbp, %rsp
        0x48, 0x89, 0xe5,
    // call *%r14
        0x41, 0xff, 0xd6,
    // pop %rbp
        0x5d,
    // pop %r11
        0x41, 0x5b,
    // pop %r10
        0x41, 0x5a,
    });
    if constexpr (std::is_same<CellType, char>::value) {
        buf_.write_bytes({
        // mov [r10+r11], %al
            0x43, 0x88, 0x04, 0x1a
        });
    } else if constexpr (std::is_same<CellType, short>::value) {
        buf_.write_bytes({
        // mov [r10+r11*2], %ax
            0x66, 0x43, 0x89, 0x04, 0x5a
        });
    } else {
        buf_.write_bytes({
        // mov [r10+r11*4], %eax
            0x43, 0x89, 0x04, 0x9a
        });
    }
}

template <typename CellType>
void CodeGenerator<CellType>::generateInsLoop(int loopNumber) {
    // mark start of loop
    uintptr_t loop_start = buf_.current_offset();
    if constexpr (std::is_same<CellType, char>::value) {
        buf_.write_bytes({
        // mov %r12b, [r10+r11]
            0x47, 0x8a, 0x24, 0x1a,
        // test %r12b, %r12b
            0x45, 0x84, 0xe4
        });
    } else if constexpr (std::is_same<CellType, short>::value) {
        buf_.write_bytes({
        // mov %r12w, [r10+4*r11]
            0x66, 0x47, 0x8b, 0x24, 0x5a,
        // test %r12w, %r12w
            0x66, 0x45, 0x85, 0xe4
        });
    } else {
        buf_.write_bytes({
        // mov %r12d, [r10+4*r11]
            0x47, 0x8b, 0x24, 0x9a,
        // test %r12d, %r12d
            0x45, 0x85, 0xe4
        });
    }
    // store patch_loc
    uintptr_t patch_loc = buf_.current_offset();
    buf_.write_bytes({
    // jz 0
        0x0f, 0x84, 0x00, 0x00, 0x00, 0x00
    });
    // add loop start info to loopStarts_
    loopStarts_.emplace(loopNumber, std::make_pair(loop_start, patch_loc));
}

template <typename CellType>
void CodeGenerator<CellType>::generateInsMul(int offset, CellType multFactor) {
    // map ins.a_ into [0, BFMEM_LENGTH), needed because of C++'s % weirdness with negative numbers
    const size_t destOffset = ((offset % BFMEM_LENGTH) + BFMEM_LENGTH) % BFMEM_LENGTH;
    /// Strategy:
    /// load index of remote into rcx
    buf_.write_bytes({
    // lea %ecx, [r11d+$destOffset]
        0x67, 0x41, 0x8d, 0x8b
    });
    buf_.write_val((uint32_t)destOffset);
    if (IS_POW_2_MEM_LENGTH) {
        buf_.write_bytes({
        // and %ecx, %r15d
            0x44, 0x21, 0xf9
        });
    } else {
        /// edx -= r15d * (edx >= r15d);
        /// This works because at this point, we know that 0 <= r11d < 2*r15d - 1
        buf_.write_bytes({
        // xor %eax, %eax
            0x31, 0xc0,
        // cmp %ecx, %r15d
            0x44, 0x39, 0xf9,
        // cmovge %eax, %r15d
            0x41, 0x0f, 0x4d, 0xc7,
        // sub %ecx, %eax
            0x29, 0xc1
        });
    }
    if constexpr (std::is_same<CellType, char>::value) {
        if (multFactor == 1) {
            buf_.write_bytes({
            /// load current cell into al
            // mov %al, [r10+r11]
                0x43, 0x8a, 0x04, 0x1a,
            });
        } else if (multFactor == -1) {
            buf_.write_bytes({
            /// load current cell into al, then negate it
            // mov %al, [r10+r11]
                0x43, 0x8a, 0x04, 0x1a,
            // neg %al
                0xf6, 0xd8
            });
        } else {
            buf_.write_bytes({
            /// load current cell into r12
            // mov %r12b, [r10+r11]
                0x47, 0x8a, 0x24, 0x1a,
            /// mult r12 by multFactor, store in al
            // mov %al, multFactor
                0xb0, (unsigned char)multFactor,
            // mul r12b
                0x41, 0xf6, 0xe4
            });
        }
        buf_.write_bytes({
        /// add value from remote
        // add %al, [r10+rcx]
            0x41, 0x02, 0x04, 0x0a,
        /// store at remote
        // mov [r10+rcx], %al
            0x41, 0x88, 0x04, 0x0a
        });
    } else if constexpr (std::is_same<CellType, short>::value) {
        if (multFactor == 1) {
            buf_.write_bytes({
            /// load current cell into ax
            // mov %ax, [r10+r11*2]
                0x66, 0x43, 0x8b, 0x04, 0x5a
            });
        } else if (multFactor == -1) {
            buf_.write_bytes({
            /// load current cell into ax, then negate it
            // mov %ax, [r10+r11*2]
                0x66, 0x43, 0x8b, 0x04, 0x5a,
            // neg %ax
                0x66, 0xf7, 0xd8
            });
        } else {
            buf_.write_bytes({
            /// load current cell into r12
            // mov %r12w, [r10+r11*2]
                0x66, 0x47, 0x8b, 0x24, 0x5a,
            /// mult r12 by multFactor, store in ax
            // mov %ax, multFactor
                0x66, 0xb8,
            });
            buf_.write_val((CellType)multFactor);
            buf_.write_bytes({
            // mul r12w
                0x66, 0x41, 0xf7, 0xe4
            });
        }
        buf_.write_bytes({
        /// add value from remote
        // add %ax, [r10+rcx*2]
            0x66, 0x41, 0x03, 0x04, 0x4a,
        /// store at remote
        // mov [r10+rcx*2], %ax
            0x66, 0x41, 0x89, 0x04, 0x4a
        });
    } else {
        if (multFactor == 1) {
            buf_.write_bytes({
            /// load current cell into al
            // mov %eax, [r10+r11*4]
                0x43, 0x8b, 0x04, 0x9a
            });
        } else if (multFactor == -1) {
            buf_.write_bytes({
            /// load current cell into al, then negate it
            // mov %eax, [r10+r11*4]
                0x43, 0x8b, 0x04, 0x9a,
            // neg %eax
                0xf7, 0xd8
            });
        } else {
            buf_.write_bytes({
            /// load current cell into r12
            // mov %r12d, [r10+r11*4]
                0x47, 0x8b, 0x24, 0x9a,
            /// mult r12 by multFactor, store in al
            // mov %eax, multFactor
                0xb8
            });
            buf_.write_val((CellType)multFactor);
            buf_.write_bytes({
            // mul r12d
                0x41, 0xf7, 0xe4
            });
        }
        buf_.write_bytes({
        /// add value from remote
        // add %al, [r10+rdx]
            0x41, 0x03, 0x04, 0x8a,
        /// store at remote
        // mov [r10+rdx], %al
            0x41, 0x89, 0x04, 0x8a
        });
    }
}

template <typename CellType>
void CodeGenerator<CellType>::generateInsOut() {
    buf_.write_bytes({
    // push %r10
        0x41, 0x52,
    // push %r11
        0x41, 0x53,
    // push %rbp
        0x55,
    // mov %rbp, %rsp
        0x48, 0x89, 0xe5,
    // xor %edi, %edi
        0x31, 0xff
    });
    if constexpr (std::is_same<CellType, char>::value) {
        buf_.write_bytes({
        // mov %dil, [r10+r11]
            0x43, 0x8a, 0x3c, 0x1a,
        });
    } else if constexpr (std::is_same<CellType, short>::value) {
        buf_.write_bytes({
        // mov %dil, [r10+r11*2]
            0x43, 0x8a, 0x3c, 0x5a,
        });
    } else {
        buf_.write_bytes({
        // mov %dil, [r10+r11*4]
            0x43, 0x8a, 0x3c, 0x9a,
        });
    }
    buf_.write_bytes({
    // call *%r13
        0x41, 0xff, 0xd5,
    // pop %rbp
        0x5d,
    // pop %r11
        0x41, 0x5b,
    // pop %r10
        0x41, 0x5a
    });
}

template <typename CellType>
void CodeGenerator<CellType>::generateInsConst(int constant) {
    if constexpr (std::is_same<CellType, char>::value) {
        buf_.write_bytes({
        // movb [r10+r11], $constant
            0x43, 0xc6, 0x04, 0x1a
        });
    } else if constexpr (std::is_same<CellType, short>::value) {
        buf_.write_bytes({
        // movw [r10+r11*2], $constant
            0x66, 0x43, 0xc7, 0x04, 0x5a
        });
    } else {
        buf_.write_bytes({
        // movl [r10+r11*4], $constant
            0x43, 0xc7, 0x04, 0x9a
        });
    }
    buf_.write_val((CellType)constant);
}

template <typename CellType>
void CodeGenerator<CellType>::generateEpilogue() {
    /// Restore callee-saved registers
    buf_.write_bytes({
    // pop %r15
        0x41, 0x5f,
    // pop %r14
        0x41, 0x5e,
    // pop %r13
        0x41, 0x5d,
    // pop %r12
        0x41, 0x5c,
    /// Return from function
    // ret
        0xc3
    });
}

template <typename CellType>
void CodeGenerator<CellType>::enter(ASMBufOffset offset) {
    buf_.set_executable(true);
    buf_.enter(offset);
}

template <typename CellType>
std::string CodeGenerator<CellType>::instructionHexDump() const {
    return buf_.instructionHexDump();
}

template <typename CellType>
size_t CodeGenerator<CellType>::generatedLength() const {
    return buf_.current_offset();
}

template class CodeGenerator<char>;
template class CodeGenerator<short>;
template class CodeGenerator<int>;
