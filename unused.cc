enum OperationCode {
    RET,
    SYSCALL,
    MOV,
    INC,
    DEC
};

enum OperandType {
    LITERAL,
    REGISTER
};

enum OperandWidth {
    BYTE,
    WORD,
    DWORD,
    QWORD
};

struct Operand {
    long long value;
    OperandType op_type;
};

struct Instruction {
    OperationCode code;
    OperandWidth op_width;
    Instruction(
        OperationCode _code,
        OperandWidth _op_width=OperandWidth::DWORD
    ): 
        code(_code),
        op_width(_op_width) {
    }
    void write_to_buf(ASMBuf &buf) const {
        switch(code) {
            case OperationCode::RET: 
                buf.write_bytes({0xc3});
                break;
            case OperationCode::SYSCALL:
                buf.write_bytes({0x0f, 0x05});
                break;
        }
    }
};
