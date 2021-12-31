#ifndef _ISA_H_
#define _ISA_H_ 1

#include <cassert>

extern "C"
{
    struct R_TYPE
    {
        uint32_t opcode : 7;
        uint32_t rd     : 5;
        uint32_t funct3 : 3;
        uint32_t rs1    : 5;
        uint32_t rs2    : 5;
        uint32_t funct7 : 7;
    };

    static_assert(sizeof(R_TYPE) == sizeof(uint32_t));

    struct I_TYPE
    {
        uint32_t opcode : 7;
        uint32_t rd     : 5;
        uint32_t funct3 : 3;
        uint32_t rs1    : 5;
        int32_t  imm    : 12;
    };

    static_assert(sizeof(I_TYPE) == sizeof(uint32_t));

    struct U_TYPE
    {
        uint32_t opcode : 7;
        uint32_t rd     : 5;
        int32_t  imm    : 20;
    };

    static_assert(sizeof(U_TYPE) == sizeof(uint32_t));

    struct S_TYPE
    {
        uint32_t opcode : 7;
        uint32_t imm5   : 5;
        uint32_t funct3 : 3;
        uint32_t rs1    : 5;
        uint32_t rs2    : 5;
        uint32_t imm7   : 7;
    };

    static_assert(sizeof(S_TYPE) == sizeof(uint32_t));

    struct B_TYPE
    {
        uint32_t opcode : 7;
        uint32_t imm11  : 1;
        uint32_t imm4   : 4;
        uint32_t funct3 : 3;
        uint32_t rs1    : 5;
        uint32_t rs2    : 5;
        uint32_t imm6   : 6;
        uint32_t imm12  : 1;
    };

    static_assert(sizeof(B_TYPE) == sizeof(uint32_t));

    struct J_TYPE
    {
        uint32_t opcode   : 7;
        uint32_t rd       : 5;
        uint32_t imm12_19 : 8;
        uint32_t imm11    : 1;
        uint32_t imm1_10  : 10;
        uint32_t imm20    : 1;
    };

    static_assert(sizeof(J_TYPE) == sizeof(uint32_t));

    struct ControlUnitFlags
    {
        uint32_t ALUOP : 3;
        uint32_t SRC2  : 3;

        uint32_t REG_WEN  : 1; // REG Write Enable
        uint32_t MEM_WEN  : 1; // MEM Write Enable
        uint32_t MEM2REG  : 1; // DMEM RD -> REG FILE
        uint32_t BRN_COND : 1; // B*?
    };

    static_assert(sizeof(ControlUnitFlags) == sizeof(uint32_t));
}


union INSTRUCTION
{
    ControlUnitFlags flags;
    uint32_t         raw;
    R_TYPE           r_type;
    I_TYPE           i_type;
    U_TYPE           u_type;
    S_TYPE           s_type;
    B_TYPE           b_type;
    J_TYPE           j_type;

    operator ControlUnitFlags() const
    { return flags; }
    operator uint32_t() const
    { return raw; }
    operator R_TYPE() const
    { return r_type; }
    operator I_TYPE() const
    { return i_type; }
    operator U_TYPE() const
    { return u_type; }
    operator S_TYPE() const
    { return s_type; }
    operator B_TYPE() const
    { return b_type; }
    operator J_TYPE() const
    { return j_type; }

    INSTRUCTION():
        raw(0)
    {}
    INSTRUCTION(ControlUnitFlags flags):
        flags(flags)
    {}
    INSTRUCTION(uint32_t value):
        raw(value)
    {}
    INSTRUCTION(R_TYPE instruction):
        r_type(instruction)
    {}
    INSTRUCTION(I_TYPE instruction):
        i_type(instruction)
    {}
    INSTRUCTION(U_TYPE instruction):
        u_type(instruction)
    {}
    INSTRUCTION(S_TYPE instruction):
        s_type(instruction)
    {}
    INSTRUCTION(B_TYPE instruction):
        b_type(instruction)
    {}
    INSTRUCTION(J_TYPE instruction):
        j_type(instruction)
    {}

    uint32_t opcode() const
    { return raw & 0x7f; }
};

static_assert(sizeof(INSTRUCTION) == sizeof(uint32_t));

extern "C" INSTRUCTION MakeADDI(size_t rd, size_t rs1, int32_t imm)
{
    assert(sizeof(I_TYPE) == sizeof(uint32_t));
    assert((-2048 <= imm) && (imm < 2048));
    assert(rd  < 32);
    assert(rs1 < 32);

    I_TYPE retval;
    retval.opcode = 0x13;
    retval.rd     = rd;
    retval.rs1    = rs1;
    retval.funct3 = 0;
    retval.imm    = imm;

    return retval;
}

extern "C" INSTRUCTION MakeADD(size_t rd, size_t rs1, size_t rs2)
{
    assert(sizeof(R_TYPE) == sizeof(uint32_t));
    assert(rd  < 32);
    assert(rs1 < 32);
    assert(rs2 < 32);

    R_TYPE retval;
    retval.opcode = 0x33;
    retval.rd     = rd;
    retval.rs1    = rs1;
    retval.rs2    = rs2;
    retval.funct3 = 0;
    retval.funct7 = 0;

    return retval;
}

extern "C" INSTRUCTION MakeSUB(size_t rd, size_t rs1, size_t rs2)
{
    assert(sizeof(R_TYPE) == sizeof(uint32_t));
    assert(rd  < 32);
    assert(rs1 < 32);
    assert(rs2 < 32);

    R_TYPE retval;
    retval.opcode = 0x33;
    retval.rd     = rd;
    retval.rs1    = rs1;
    retval.rs2    = rs2;
    retval.funct3 = 0;
    retval.funct7 = 1 << 5;
    return retval;
}

extern "C" INSTRUCTION MakeBEQ(size_t rs1, size_t rs2, int32_t delta)
{
    assert(rs1 < 32);
    assert(rs2 < 32);
    assert((-4096 <= delta) && (delta < 4096));

    B_TYPE retval;
    retval.opcode = 0x63;
    retval.funct3 = 0x0;
    retval.imm12  = ((delta & 0x1000) >> 12);
    retval.imm11  = ((delta & 0x800)  >> 11);
    retval.imm4   = ((delta & 0x1E)   >> 1);
    retval.imm6   = ((delta & 0x7E0)  >> 5);
    retval.rs1    = rs1;
    retval.rs2    = rs2;
    return retval;
}

extern "C" INSTRUCTION MakeBNE(size_t rs1, size_t rs2, int32_t delta)
{
    assert(rs1 < 32);
    assert(rs2 < 32);
    assert((-4096 <= delta) && (delta < 4096));

    B_TYPE retval;
    retval.opcode = 0x63;
    retval.funct3 = 0x1;
    retval.imm12  = ((delta & 0x1000) >> 12);
    retval.imm11  = ((delta & 0x800)  >> 11);
    retval.imm4   = ((delta & 0x1E)   >> 1);
    retval.imm6   = ((delta & 0x7E0)  >> 5);
    retval.rs1    = rs1;
    retval.rs2    = rs2;
    return retval;
}

#endif // _ISA_H_
