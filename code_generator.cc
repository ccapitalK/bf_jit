#include <iostream>
#include <optional>
#include <stack>

#include "code_generator.hpp"
#include "runtime.hpp"

// Instructions in these comments use intel syntax
//
// Register model:
// r10 = &bfMem[0]
// r11 is the offset into bfMem_
// r12 is sometimes used to store the value of the current cell
// r13 is the address of mputchar
// r14 is the address of mgetc
// r15 is (BFMEM_LENGTH-1) if IS_POW_2_MEM_LENGTH, else it is BFMEM_LENGTH

ASMBufOffset CodeGenerator::compile(const std::vector<Instruction> &prog) {
    buf_.set_executable(false);
    const auto startOffset = buf_.current_offset();
    generatePrelude();
    for (auto ins : prog) {
        // std::cout << "Instruction: " << ins << '\n';
        switch (ins.code_) {
        case IROpCode::INS_ADD:
            generateInsAdd(ins.a_);
            break;
        case IROpCode::INS_ADP:
            generateInsAdp(ins.a_);
            break;
        case IROpCode::INS_MUL:
            generateInsMul(ins.a_, ins.b_);
            break;
        case IROpCode::INS_ZERO:
            generateInsZero();
            break;
        case IROpCode::INS_OUT:
            generateInsOut();
            break;
        case IROpCode::INS_IN:
            generateInsIn();
            break;
        case IROpCode::INS_LOOP:
            generateInsLoop(ins.a_);
            break;
        case IROpCode::INS_END_LOOP:
            generateInsEndLoop(ins.a_);
            break;
        default:
            throw JITError("ICE: Unhandled instruction");
        }
    }
    generateEpilogue();
    return startOffset;
}

void CodeGenerator::generatePrelude() {
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
    buf_.write_val((uintptr_t)mputchar);

    buf_.write_bytes({
    // mov %r14, mgetc
        0x49, 0xbe
    });
    buf_.write_val((uintptr_t)mgetc);

    buf_.write_bytes({
    // mov %r15, $BFMEM_LENGTH
        0x49, 0xbf
    });
    buf_.write_val((size_t)BFMEM_LENGTH - (size_t)IS_POW_2_MEM_LENGTH);
}

void CodeGenerator::generateInsAdd(unsigned char step) {
    buf_.write_bytes({
    // mov %r12b, [r10+r11]
        0x47, 0x8a, 0x24, 0x1a,
    // add %r12b, $step
        0x41, 0x80, 0xc4, step,
    // mov [r10+r11], %r12b
        0x47, 0x88, 0x24, 0x1a
    });
}

void CodeGenerator::generateInsAdp(int step) {
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

void CodeGenerator::generateInsEndLoop(int loopNumber) {
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

void CodeGenerator::generateInsIn() {
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
    // mov [r10+r11], %al
        0x43, 0x88, 0x04, 0x1a
    });
}

void CodeGenerator::generateInsLoop(int loopNumber) {
    // mark start of loop
    uintptr_t loop_start = buf_.current_offset();
    buf_.write_bytes({
    // mov %r12b, [r10+r11]
        0x47, 0x8a, 0x24, 0x1a,
    // test %r12b, %r12b
        0x45, 0x84, 0xe4
    });
    // store patch_loc
    uintptr_t patch_loc = buf_.current_offset();
    buf_.write_bytes({
    // jz 0
        0x0f, 0x84, 0x00, 0x00, 0x00, 0x00
    });
    // add loop start info to loopStarts_
    loopStarts_.emplace(loopNumber, std::make_pair(loop_start, patch_loc));
}

void CodeGenerator::generateInsMul(int offset, char multFactor) {
    // map ins.a_ into [0, BFMEM_LENGTH), needed because of C++'s % weirdness with negative numbers
    const size_t destOffset = ((offset % BFMEM_LENGTH) + BFMEM_LENGTH) % BFMEM_LENGTH;
    /// Strategy:
    /// load offset of remote into rax
    buf_.write_bytes({
    // mov %eax, %r11d
        0x44, 0x89, 0xd8,
    // add %eax, $destOffset
        0x05
    });
    buf_.write_val((uint32_t)destOffset);
    if (IS_POW_2_MEM_LENGTH) {
        buf_.write_bytes({
        // and %eax, %r15d
            0x44, 0x21, 0xf8
        });
    } else {
        /// eax -= r15d * (eax >= r15d);
        /// This works because at this point, we know that 0 <= r11d < 2*r15d - 1
        buf_.write_bytes({
        // mov %edx, %edx
            0x31, 0xd2,
        // cmp %eax, %r15d
            0x44, 0x39, 0xf8,
        // cmovge %r15d, %edx
            0x41, 0x0f, 0x4d, 0xd7,
        // sub %eax, %edx
            0x29, 0xd0
        });
    }
    buf_.write_bytes({
    // mov %rdx, %rax
        0x48, 0x89, 0xc2,
    /// load current cell into r12
    // mov %r12b, [r10+r11]
        0x47, 0x8a, 0x24, 0x1a,
    /// mult r12 by multFactor, store in al
    // mov %al, multFactor
        0xb0, (unsigned char)multFactor,
    // mul r12b
        0x41, 0xf6, 0xe4,
    /// load value at remote
    // mov %r12b, [r10+rdx]
        0x45, 0x8a, 0x24, 0x12,
    /// add together
    // add %al, %r12b
        0x44, 0x00, 0xe0,
    /// store at remote
    // mov [r10+rdx], %al
        0x41, 0x88, 0x04, 0x12,
    });
}

void CodeGenerator::generateInsOut() {
    buf_.write_bytes({
    // push %r10
        0x41, 0x52,
    // push %r11
        0x41, 0x53,
    // push %rbp
        0x55,
    // mov %rbp, %rsp
        0x48, 0x89, 0xe5,
    // mov %dil, [r10+r11]
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
}

void CodeGenerator::generateInsZero() {
    buf_.write_bytes({
    // xor %r12, %r12
        0x4d, 0x31, 0xe4,
    // mov [r10+r11], %r12b
        0x47, 0x88, 0x24, 0x1a
    });
}

void CodeGenerator::generateEpilogue() {
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

void CodeGenerator::enter(ASMBufOffset offset) {
    buf_.set_executable(true);
    buf_.enter(offset);
}

std::string CodeGenerator::instructionHexDump() const {
    return buf_.instructionHexDump();
}

size_t CodeGenerator::generatedLength() const {
    return buf_.current_offset();
}
