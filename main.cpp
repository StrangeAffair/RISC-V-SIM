#include <iostream>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <unordered_map>
#include <vector>

#include "ISA.h"


size_t GLOBAL_STAGE = 0;

class BaseBlock
{
public:
    static constexpr const char* TypeName = "BaseBlock";

public:
    virtual const char* Type() const
    { return TypeName; }

    virtual void step() = 0;
};

class Wire : public BaseBlock
{
public:
    static constexpr const char* TypeName = "Wire";

public:
    const char* Type() const override
    { return TypeName; }

    void step() override
    { stage = GLOBAL_STAGE; }

public:
    Wire(const char* name = nullptr):
        name (name),
        value(0),
        stage(GLOBAL_STAGE)
    {}

public:
    const char* GetName() const
    { return name; }

public:
    template<class T = uint32_t>
    T OldValue() const
    {
        return static_cast<T>(value);
    }

    template<class T = uint32_t>
    T GetValue()
    {
        if (stage != GLOBAL_STAGE)
            step();
        return static_cast<T>(value);
    }

    template<class T>
    void SetValue(T __value)
    { value = __value; }

public:
    Wire& operator=(const Wire& other)
    {
        if (this == &other)
            return *this;

        value = other.value;
        return *this;
    }
    Wire& operator=(INSTRUCTION instruction)
    {
        value = instruction.raw;
        return *this;
    }
    Wire& operator=(uint32_t __value)
    {
        value = __value;
        return *this;
    }

public:
    operator uint32_t()
    { return GetValue(); }

//protected:
    const char* name;
    uint32_t    value;
    size_t      stage;
};

class FlipFlop : public Wire
{
public:
    static constexpr const char* TypeName = "FlipFlop";

public:
    const char* Type() const override
    { return TypeName; }

    void step() override
    {
        if (stage != GLOBAL_STAGE)
        {
            value = input->OldValue();
            stage = GLOBAL_STAGE;
        }
    }

public:
    FlipFlop(Wire* input, const char* name = nullptr):
        Wire (name),
        input(input)
    {}

public:
    Wire* input;
};

std::unordered_map<const char*, Wire*> Wires;

Wire* GetWire(const char* name);

void FillWires()
{
    // Fetch FlipFlop (before fetch stage)
    Wires["Fetch FlipFlop IN"]  = new Wire    ("PC_NEXT");
    Wires["Fetch FlipFlop OUT"] = new FlipFlop(Wires["Fetch FlipFlop IN"], "PC");

    // Fetch NextInstruction
    Wires["PC"]      = Wires["Fetch FlipFlop OUT"];
    Wires["PC_EX"]   = new Wire("PC_EX");
    Wires["PC_DISP"] = new Wire("PC_DISP");
    Wires["PC_R"]    = new Wire("PC_R");
    Wires["PC_NEXT"] = Wires["Fetch FlipFlop IN"];

    // Fetch IMEM
    Wires["IMEM A"] = Wires["PC"];
    Wires["IMEM D"] = new Wire("IMEM D");

    // Decode PC_DE
    Wires["PC_DE"] = new FlipFlop(Wires["PC"], "PC_DE");

    // Decode FlipFlop (before decode stage)
    Wires["Decode FlipFlop INSTR IN"]  = Wires["IMEM D"];
    Wires["Decode FlipFlop INSTR OUT"] = new FlipFlop(Wires["Decode FlipFlop INSTR IN"], "INSTRUCTION");
    Wires["INSTRUCTION"] = Wires["Decode FlipFlop INSTR OUT"];

    Wires["Decode FlipFlop PC IN"]  = Wires["PC"];
    Wires["Decode FlipFlop PC OUT"] = new Wire();

    Wires["Decode FlipFlop PC_R IN"]  = Wires["PC_R"];
    Wires["Decode FlipFlop PC_R OUT"] = new FlipFlop(Wires["Decode FlipFlop PC_R IN"], "PC_RF");
    Wires["PC_RF"] = Wires["Decode FlipFlop PC_R OUT"];
    Wires["PC_RD"] = Wires["PC_R"];

    // Decode RegFile
    Wires["Decode RegFile INSTR"] = Wires["INSTRUCTION"];
    Wires["WB_A"]  = new Wire("WB_A");
    Wires["WB_D"]  = new Wire("WB_D");
    Wires["WB_WE"] = new Wire("WB_WE");
    Wires["RS1"]   = new Wire("RS1");
    Wires["RS2"]   = new Wire("RS2");

    // Decode CU
    Wires["Decode CU INSTR"] = Wires["INSTRUCTION"];
    Wires["Decode CU FLAGS"] = new Wire();
    Wires["CU FLAGS"] = Wires["Decode CU FLAGS"];

    Wires["V_DE"] = new Wire("V_DE");

    // Execute
    Wires["V_EX"]                = new FlipFlop(Wires["V_DE"],        "V_EX");
    Wires["CONTROL_EX"]          = new FlipFlop(Wires["CU FLAGS"],    "CONTROL_EX");
    Wires["Execute RS1"]         = new FlipFlop(Wires["RS1"],         "RS1_EX");
    Wires["Execute RS2"]         = new FlipFlop(Wires["RS2"],         "RS2_EX");
    Wires["Execute INSTRUCTION"] = new FlipFlop(Wires["INSTRUCTION"], "INSTR_EX");
    Wires["PC_EX"]               = new FlipFlop(Wires["PC_DE"],       "PC_EX");

    Wires["WE_GEN WB_WE"]  = new Wire("Execute WB_WE");
    Wires["WE_GEN MEM_WE"] = new Wire("Execute MEM_WE");

    Wires["HU_RS1"] = new Wire("HU_RS1");
    Wires["HU_RS2"] = new Wire("HU_RS2");
    Wires["RS1V"] = new Wire("RS1V");
    Wires["RS2V"] = new Wire("RS2V");
    Wires["SRC2"] = new Wire("SRC2");
    Wires["IMM VALUE 1"] = new Wire("IMM VALUE 1");
    Wires["IMM VALUE 2"] = new Wire("IMM VALUE 2");
    Wires["IMM VALUE 3"] = new Wire("IMM VALUE 3");
    Wires["IMM VALUE 4"] = new Wire("IMM VALUE 4");
    Wires["IMM VALUE 5"] = new Wire("IMM VALUE 5");

    Wires["ALU LEFT"]   = Wires["RS1V"];
    Wires["ALU RIGHT"]  = Wires["SRC2"];
    Wires["ALU RESULT"] = new Wire("ALU RESULT");

    Wires["CMP LEFT"]   = Wires["RS1V"];
    Wires["CMP RIGHT"]  = Wires["RS2V"];
    Wires["CMP RESULT"] = new Wire("CMP RESULT");

    // Memory
    Wires["Memory WE_GEN WB_WE"]  = new FlipFlop(Wires["WE_GEN WB_WE"],  "Memory WE_GEN WB_WE");
    Wires["Memory WE_GEN MEM_WE"] = new FlipFlop(Wires["WE_GEN MEM_WE"], "MEM_WE");
    Wires["MEM_WE"] = Wires["Memory WE_GEN MEM_WE"];

    Wires["Memory CONTROL_EX"]  = new FlipFlop(Wires["CONTROL_EX"],          "Memory CONTROL_EX");
    Wires["Memory RS1"]         = new FlipFlop(Wires["Execute RS1"],         "Memory RS1");
    Wires["Memory ALU"]         = new FlipFlop(Wires["ALU RESULT"],          "Memory ALU");
    Wires["Memory INSTRUCTION"] = new FlipFlop(Wires["Execute INSTRUCTION"], "Memory INSTRUCTION");

    Wires["DMEM WE"] = Wires["MEM_WE"];
    Wires["DMEM WD"] = Wires["Memory RS1"];
    Wires["DMEM A"]  = Wires["Memory ALU"];
    Wires["DMEM RD"] = new Wire("DMEM RD");

    Wires["BP_MEM"]  = Wires["Memory ALU"];

    Wires["Memory WB_D"] = new Wire();

    Wires["Memory HU_MEM_RD"] = Wires["Memory INSTRUCTION"];

    // Write Back
    Wires["WB CONTROL_EX"] = new FlipFlop(Wires["Memory CONTROL_EX"], "WB CONTROL_EX");

    Wires["WB_WE"] = new FlipFlop(GetWire("Memory WE_GEN WB_WE"), "WB_WE");
    Wires["WB_D"]  = new FlipFlop(GetWire("Memory WB_D"),         "WB_D");
    Wires["WB_A"]  = new FlipFlop(GetWire("Memory INSTRUCTION"),  "WB_A");

    Wires["BP_WB"]        = Wires["WB_D"];
    Wires["WB HU_MEM_RD"] = Wires["WB_A"];
}

Wire* GetWire(const char* name)
{
    if (Wires.find(name) == Wires.end())
    {
        std::cerr << "bad wire = " << name << std::endl;
        throw "bad wire";
    }

    return Wires[name];
}

void PrintWires();

class InstructionMemory : public BaseBlock
{
public:
    static constexpr const char* TypeName = "InstructionMemory";

public:
    const char* Type() const override
    { return TypeName; }

    void step() override
    {
        size_t offset = address->GetValue() >> 2;
        if (offset < size)
        {
            *instruction = memory[offset];
        }
        else
        {
            throw "InstructionMemory bad address";
        }
    }

public:
    InstructionMemory():
        address    (GetWire("IMEM A")),
        instruction(GetWire("IMEM D")),
        memory(nullptr),
        size  (0)
    {}

    ~InstructionMemory()
    {
        if (memory != nullptr)
            delete memory;
    }

    size_t SetMemory(INSTRUCTION* array, size_t size)
    {
        INSTRUCTION* ptr = new (std::nothrow) INSTRUCTION[size];
        if (ptr == nullptr)
            return 0;

        memcpy(ptr, array, size * sizeof(INSTRUCTION));
        this->memory = ptr;
        this->size   = size;
        return size;
    }

public:
    Wire* address;
    Wire* instruction;

private:
    INSTRUCTION* memory;
    size_t       size;
};

class NextInstruction : public BaseBlock
{
public:
    static constexpr const char* TypeName = "NextInstruction";

public:
    const char* Type() const override
    { return TypeName; }

    void step() override
    {
        if (!PC_R->GetValue<bool>())
        { *PC_NEXT = *PC + 4; }
        else
        { *PC_NEXT = *PC_EX + *PC_DISP; }
    }

public:
    NextInstruction():
        PC     (GetWire("PC")),
        PC_R   (GetWire("PC_R")),
        PC_EX  (GetWire("PC_EX")),
        PC_DISP(GetWire("PC_DISP")),
        PC_NEXT(GetWire("PC_NEXT"))
    {}

public:
    Wire* PC;
    Wire* PC_R;
    Wire* PC_EX;
    Wire* PC_DISP;

public:
    Wire* PC_NEXT;
};

class ControlUnit : public BaseBlock
{
public:
    static constexpr const char* TypeName = "ControlUnit";

public:
    const char* Type() const override
    { return TypeName; }

    void step() override
    {
        INSTRUCTION      instruction(*raw_instruction);
        ControlUnitFlags flags;

        uint32_t opcode  = instruction.opcode();
        uint32_t command = opcode >> 2;
        if ((opcode & 0x3) != 0x3)
            throw "not in RV32I Base Instruction Set";

        /** ALU_SRC2:
                0 = R-type
                1 = I-type  (imm[11:0])
                2 = S-type  (imm[11:5]    + imm[4:0])
                3 = SB-type (imm[12|10:5] + imm[4:1|11])
                4 = U-type  (imm[31:12])
                5 = UJ-type (imm[20|10:1|11|19:12])

            funct3[14:12] field always represent operation
        */
        switch (command)
        {
        case 0x0d: // LUI   (load the upper 20 bits) (rd = imm)
            throw "LUI is not supported";   // can not get imm directly
            flags.ALUOP = 0;
            flags.SRC2  = 4; // imm[31:12]

            flags.REG_WEN  = true;  // REG Write Enable
            flags.MEM_WEN  = false; // MEM Write Enable
            flags.MEM2REG  = false; // DMEM RD -> REG FILE
            flags.BRN_COND = false; // B*?
            break;
        case 0x05: // AUIPC (add upper immediate to pc) (rd = PC + imm)
            throw "AUIPC is not supported"; // can not use ALU for rs1 == PC
            flags.ALUOP = 0;
            flags.SRC2  = 4; // imm[31:12]

            flags.REG_WEN  = true;  // REG Write Enable
            flags.MEM_WEN  = false; // MEM Write Enable
            flags.MEM2REG  = false; // DMEM RD -> REG FILE
            flags.BRN_COND = false; // B*?
            break;
        case 0x1b: // JAL   (rd = PC + 4, PC = PC + imm)
            throw "JAL is not supported"; // how to do rd = PC + 4?
            flags.ALUOP = 0;
            flags.SRC2  = 5; // imm[20|10:1|11|19:12]

            flags.REG_WEN  = true;  // REG Write Enable
            flags.MEM_WEN  = false; // MEM Write Enable
            flags.MEM2REG  = false; // DMEM RD -> REG FILE
            flags.BRN_COND = true;  // B*?
            break;
        case 0x19: // JALR  (rd = PC + 4, PC = rs1 + imm)
            throw "JALR is not supported"; // can not store ALU result in PC
            flags.ALUOP = 0;
            flags.SRC2  = 1; // imm[11:0]

            flags.REG_WEN  = true;  // REG Write Enable
            flags.MEM_WEN  = false; // MEM Write Enable
            flags.MEM2REG  = false; // DMEM RD -> REG FILE
            flags.BRN_COND = true;  // B*?
            break;
        case 0x18: // B*    (if (rs1 op rs2) then (PC += imm) else (PC += 4))
            flags.ALUOP = instruction.r_type.funct3;
            flags.SRC2  = 3; // imm[12|10:5] + imm[4:1|11]

            flags.REG_WEN  = false; // REG Write Enable
            flags.MEM_WEN  = false; // MEM Write Enable
            flags.MEM2REG  = false; // DMEM RD -> REG FILE
            flags.BRN_COND = true;  // B*?
            break;
        case 0x00: // L{B,H,W}{_,U}
            flags.ALUOP = instruction.r_type.funct3;
            flags.SRC2  = 1; // imm[11:0]

            flags.REG_WEN  = true;  // REG Write Enable
            flags.MEM_WEN  = false; // MEM Write Enable
            flags.MEM2REG  = true;  // DMEM RD -> REG FILE
            flags.BRN_COND = false; // B*?
            break;
        case 0x08: // S{B,H,W}
            flags.ALUOP = instruction.r_type.funct3;
            flags.SRC2  = 2; // imm[11:5] + imm[4:0]

            flags.REG_WEN  = false; // REG Write Enable
            flags.MEM_WEN  = true;  // MEM Write Enable
            flags.MEM2REG  = false; // DMEM RD -> REG FILE
            flags.BRN_COND = false; // B*?
            break;
        case 0x04: // (OP)I (rd = rs1 op imm)
            flags.ALUOP = instruction.r_type.funct3;
            flags.SRC2  = 1; // imm[11:0]

            flags.REG_WEN  = true;  // REG Write Enable
            flags.MEM_WEN  = false; // MEM Write Enable
            flags.MEM2REG  = false; // DMEM RD -> REG FILE
            flags.BRN_COND = false; // B*?
            break;
        case 0x0c: // (OP)  (rd = rs1 op rs2)
            flags.ALUOP = instruction.r_type.funct3;
            flags.SRC2  = 0; // reg

            flags.REG_WEN  = true;  // REG Write Enable
            flags.MEM_WEN  = false; // MEM Write Enable
            flags.MEM2REG  = false; // DMEM RD -> REG FILE
            flags.BRN_COND = false; // B*?
            break;
        case 0x03: // FENCE and FENCE.I
            throw "FENCE and FENCE.I instructions are not supported";
        case 0x1c: // ECALL and EBREAK
            throw "ECALL and EBREAK instructions are not supported";
        default:   // ???
            throw "command not supported";
        }

        *CU_flags = ((INSTRUCTION) flags).raw;
    }

public:
    ControlUnit():
        raw_instruction(GetWire("Decode CU INSTR")),
        CU_flags       (GetWire("Decode CU FLAGS"))
    {}

public:
    Wire* raw_instruction;
    Wire* CU_flags;
};

class RegisterFile : public BaseBlock
{
public:
    static constexpr const char* TypeName = "RegisterFile";

public:
    const char* Type() const override
    { return TypeName; }

    void step() override
    {
        size_t rd  = INSTRUCTION(*WB_A).r_type.rd;
        size_t rs1 = INSTRUCTION(*instruction).r_type.rs1;
        size_t rs2 = INSTRUCTION(*instruction).r_type.rs2;

        if (WB_WE->GetValue<bool>() && rd != 0)
        {
            std::cout << "WB RegisterFile offset = " << rd << std::endl;
            regs[rd] = *WB_D;
        }

        std::cout << "rs1 = " << rs1 << ", rs2 = " << rs2 << std::endl;
        *RS1 = regs[rs1];
        *RS2 = regs[rs2];
    }

public:
    RegisterFile():
        instruction (GetWire("INSTRUCTION")),
        WB_A  (GetWire("WB_A")),
        WB_D  (GetWire("WB_D")),
        WB_WE (GetWire("WB_WE")),
        RS1   (GetWire("RS1")),
        RS2   (GetWire("RS2")),
        regs  {}
    {}

public:
    Wire* instruction;
    Wire* WB_A;  // write back address (inside instruction)
    Wire* WB_D;  // write back data
    Wire* WB_WE; // write back write enable

public:
    Wire* RS1;
    Wire* RS2;

public:
    uint32_t regs[32];
};

class WriteEnableGenerator : public BaseBlock
{
public:
    static constexpr const char* TypeName = "WriteEnableGenerator";

public:
    const char* Type() const override
    { return TypeName; }

    void step() override
    {
        ControlUnitFlags flags = INSTRUCTION(*CONTROL_EX).flags;

        if (!flags.BRN_COND && V_EX->GetValue<bool>())
        {
            *MEM_WE = flags.MEM_WEN;
            *WB_WE  = flags.REG_WEN;
        }
        else
        {
            *MEM_WE = false;
            *WB_WE  = false;
        }
    }

public:
    WriteEnableGenerator():
        V_EX      (GetWire("V_EX")),
        CONTROL_EX(GetWire("CONTROL_EX")),
        MEM_WE    (GetWire("WE_GEN MEM_WE")),
        WB_WE     (GetWire("WE_GEN WB_WE"))
    {}

public:
    Wire* V_EX;
    Wire* CONTROL_EX;

public:
    Wire* MEM_WE;
    Wire* WB_WE;
};

class HazardUnit : public BaseBlock
{
public:
    static constexpr const char* TypeName = "HazardUnit";

public:
    const char* Type() const override
    { return TypeName; }

    void step() override
    {
        uint32_t rs1    = INSTRUCTION(HU_EX_INSTR ->OldValue()).r_type.rs1;
        uint32_t rs2    = INSTRUCTION(HU_EX_INSTR ->OldValue()).r_type.rs2;
        uint32_t rd_mem = INSTRUCTION(HU_MEM_RDMEM->OldValue()).r_type.rd;
        uint32_t rd_wb  = INSTRUCTION(HU_MEM_RDWB ->OldValue()).r_type.rd;

        *HU_RS1 = 0x0;
        *HU_RS2 = 0x0;

        std::cout << "REG_WE_M  = " << *REG_WE_M  << '\n';
        std::cout << "REG_WE_WB = " << *REG_WE_WB << '\n';

        ControlUnitFlags flagsM = INSTRUCTION(HU_CONTROL_M->OldValue()).flags;
        if (*REG_WE_M && !flagsM.BRN_COND && flagsM.REG_WEN && !flagsM.MEM2REG)
        {
            // we only use BP_MEM (ALU result) when we will choose ALU result and write back
            if (rs1 == rd_mem)
                *HU_RS1 = 0x1;
            if (rs2 == rd_mem)
                *HU_RS2 = 0x1;
        }

        ControlUnitFlags flagsWB = INSTRUCTION(HU_CONTROL_WB->OldValue()).flags;
        if (*REG_WE_WB && !flagsWB.BRN_COND && flagsM.REG_WEN)
        {
            if (rs1 == rd_wb)
                *HU_RS1 = 0x2;
            if (rs2 == rd_wb)
                *HU_RS2 = 0x2;
        }
    }

public:
    HazardUnit():
        // not on scheme !!!
        HU_CONTROL_M (GetWire("Memory CONTROL_EX")),
        HU_CONTROL_WB(GetWire("WB CONTROL_EX")),
        REG_WE_M     (GetWire("Memory WE_GEN WB_WE")),
        REG_WE_WB    (GetWire("WB_WE")),

        HU_EX_INSTR (GetWire("Execute INSTRUCTION")),
        HU_MEM_RDMEM(GetWire("Memory HU_MEM_RD")),
        HU_MEM_RDWB (GetWire("WB HU_MEM_RD")),
        HU_RS1      (GetWire("HU_RS1")),
        HU_RS2      (GetWire("HU_RS2"))
    {}

public:
    Wire* HU_CONTROL_M;
    Wire* HU_CONTROL_WB;

    Wire* REG_WE_M;
    Wire* REG_WE_WB;

    Wire* HU_EX_INSTR;
    Wire* HU_MEM_RDMEM;
    Wire* HU_MEM_RDWB;

public:
    Wire* HU_RS1;
    Wire* HU_RS2;
};

class Immediate : public BaseBlock
{
public:
    static constexpr const char* TypeName = "Immediate";

public:
    const char* Type() const override
    { return TypeName; }

    void step() override
    {
        INSTRUCTION instr(*instruction);
        /**
            0 = R-type
            1 = I-type  (imm[11:0])
            2 = S-type  (imm[11:5]    + imm[4:0])
            3 = SB-type (imm[12|10:5] + imm[4:1|11])
            4 = U-type  (imm[31:12])
            5 = UJ-type (imm[20|10:1|11|19:12])
        */

        // I-type (imm type == int32)
        *output1 = instr.i_type.imm;

        // S-type
        {
            bool     sign  = (instr.s_type.imm7 & 0x40);
            uint32_t value = (instr.s_type.imm7 & 0x3f) + instr.s_type.imm5;

            if (!sign)
                *output2 = value;
            else
                *output2 = -((~value & 0x7ff) + 1);
        }

        // SB-type
        {
            uint32_t value = (instr.b_type.imm11 << 11) + (instr.b_type.imm6 << 5) + (instr.b_type.imm4 << 1);

            if (!instr.b_type.imm12)
                *output3 = value;
            else
                *output3 = -((~value & 0xfff) + 1);
        }

        // U-type (imm type == int32)
        *output4 = instr.u_type.imm << 12;

        // UJ-type
        {
            uint32_t value = (instr.j_type.imm12_19 << 12) + (instr.j_type.imm11 << 12) + (instr.j_type.imm1_10 << 1);

            if (!instr.j_type.imm20)
                *output5 = value;
            else
                *output5 = -((~value & 0xfffff) + 1);
        }

        *PC_DISP = *output3;
    }

public:
    Immediate():
        instruction (GetWire("Execute INSTRUCTION")),
        output1     (GetWire("IMM VALUE 1")),
        output2     (GetWire("IMM VALUE 2")),
        output3     (GetWire("IMM VALUE 3")),
        output4     (GetWire("IMM VALUE 4")),
        output5     (GetWire("IMM VALUE 5")),
        PC_DISP     (GetWire("PC_DISP"))
    {}

public:
    Wire* instruction;

public:
    Wire* output1;
    Wire* output2;
    Wire* output3;
    Wire* output4;
    Wire* output5;
    Wire* PC_DISP;
};

class RS_TO_RSV : public BaseBlock
{
public:
    static constexpr const char* TypeName = "RS_TO_RSV";

public:
    const char* Type() const override
    { return TypeName; }

    void step() override
    {
        switch(*HU_RS)
        {
        case 0:
            *RSV = *RS;
            break;
        case 1:
            *RSV = *BP_MEM;
            break;
        case 2:
            *RSV = *BP_WB;
            break;
        default:
            throw "bad HU_RS for RSV selector";
        }
    }

public:
    RS_TO_RSV(size_t number):
        RS    (nullptr),
        HU_RS (nullptr),
        BP_MEM(GetWire("BP_MEM")),
        BP_WB (GetWire("BP_WB"))
    {
        switch(number)
        {
            case 1:
            {
                RS    = GetWire("RS1");
                HU_RS = GetWire("HU_RS1");
                RSV   = GetWire("RS1V");
                break;
            }
            case 2:
            {
                RS    = GetWire("RS2");
                HU_RS = GetWire("HU_RS2");
                RSV   = GetWire("RS2V");
                break;
            }
            default:
                throw "bad number in RS_TO_RSV(number)";
        }
    }

public:
    Wire* RS;
    Wire* HU_RS;
    Wire* BP_MEM;
    Wire* BP_WB;

public:
    Wire* RSV;
};

class SRC2_SELECTOR : public BaseBlock
{
public:
    static constexpr const char* TypeName = "SRC2_SELECTOR";

public:
    const char* Type() const override
    { return TypeName; }

    void step() override
    {
        uint32_t ALU_SRC2 = INSTRUCTION(*CONTROL_EX).flags.SRC2;
        std::cout << "ALU_SRC2 = " << ALU_SRC2 << std::endl;

        switch (ALU_SRC2)
        {
        case 0: /// reg
            *SRC2 = *RS2V;
            break;
        case 1: /// I-type  (imm[11:0])
            *SRC2 = *IMM_VALUE_1;
            break;
        case 2: /// S-type  (imm[11:5] + imm[4:0])
            *SRC2 = *IMM_VALUE_2;
            break;
        case 3: /// SB-type (imm[12|10:5] + imm[4:1|11])
            *SRC2 = *IMM_VALUE_3;
            break;
        case 4: /// U-type  (imm[31:12])
            *SRC2 = *IMM_VALUE_4;
            break;
        case 5: /// UJ-type (imm[20|10:1|11|19:12])
            *SRC2 = *IMM_VALUE_5;
            break;
        default:
            throw "bad ALU_SRC2";
        }
    }

public:
    SRC2_SELECTOR():
        RS2V        (GetWire("RS2V")),
        IMM_VALUE_1 (GetWire("IMM VALUE 1")),
        IMM_VALUE_2 (GetWire("IMM VALUE 2")),
        IMM_VALUE_3 (GetWire("IMM VALUE 3")),
        IMM_VALUE_4 (GetWire("IMM VALUE 4")),
        IMM_VALUE_5 (GetWire("IMM VALUE 5")),
        CONTROL_EX  (GetWire("CONTROL_EX")),
        SRC2        (GetWire("SRC2"))
    {}

public:
    Wire* RS2V;
    Wire* IMM_VALUE_1;
    Wire* IMM_VALUE_2;
    Wire* IMM_VALUE_3;
    Wire* IMM_VALUE_4;
    Wire* IMM_VALUE_5;
    Wire* CONTROL_EX;

public:
    Wire* SRC2;
};

class ArithmeticLogicUnit : public BaseBlock
{
public:
    constexpr static size_t ADD  = 0;
    constexpr static size_t SLT  = 2;
    constexpr static size_t SLTU = 3;
    constexpr static size_t XOR  = 4;
    constexpr static size_t OR   = 6;
    constexpr static size_t AND  = 7;
    constexpr static size_t SL   = 1;
    constexpr static size_t SR   = 5;

public:
    static constexpr const char* TypeName = "ArithmeticLogicUnit";

public:
    const char* Type() const override
    { return TypeName; }

    void step()
    {
        uint32_t ALUOP = INSTRUCTION(*CONTROL_EX).flags.ALUOP;

        switch (ALUOP)
        {
        case ADD:
            *RESULT = (*SRC1) + (*SRC2);
            break;
        case AND:
            *RESULT = (*SRC1) & (*SRC2);
            break;
        case OR:
            *RESULT = (*SRC1) | (*SRC2);
            break;
        case XOR:
            *RESULT = (*SRC1) ^ (*SRC2);
            break;
        case SL:
            *RESULT = (*SRC1) << (*SRC2);
            break;
        case SR:
            *RESULT = (*SRC1) >> (*SRC2);
            break;
        case SLT:
            *RESULT = (int32_t) (*SRC1) < (int32_t) (*SRC2);
            break;
        case SLTU:
            *RESULT = (*SRC1) < (*SRC2);
            break;
        default:
            throw "bad ALUOP";
        }
    }

public:
    ArithmeticLogicUnit():
        SRC1      (GetWire("ALU LEFT")),
        SRC2      (GetWire("ALU RIGHT")),
        CONTROL_EX(GetWire("CONTROL_EX")),
        RESULT    (GetWire("ALU RESULT"))
    {}

public:
    Wire* SRC1;
    Wire* SRC2;
    Wire* CONTROL_EX;

public:
    Wire* RESULT;
};

class Comparator : public BaseBlock
{
public:
    static constexpr const char* TypeName = "Comparator";

public:
    const char* Type() const override
    { return TypeName; }

    void step() override
    {
        uint32_t CMPOP = INSTRUCTION(*CONTROL_EX).flags.ALUOP;

        switch (CMPOP)
        {
        case 0x0: // BEQ
            *output = ((*RS1V) == (*RS2V));
            break;
        case 0x1: // BNE
            *output = ((*RS1V) != (*RS2V));
            break;
        case 0x4: // BLT
            *output = ((int32_t) (*RS1V) <  (int32_t) (*RS2V));
            break;
        case 0x5: // BGE
            *output = ((int32_t) (*RS1V) >= (int32_t) (*RS2V));
            break;
        case 0x6: // BLTU
            *output = ((*RS1V) <  (*RS2V));
            break;
        case 0x7: // BGEU
            *output = ((*RS1V) >= (*RS2V));
            break;
        default:
            throw "bad CMPOP";
        }
    }

public:
    Comparator():
        CONTROL_EX (GetWire("CONTROL_EX")),
        RS1V       (GetWire("RS1V")),
        RS2V       (GetWire("RS2V")),
        output     (GetWire("CMP RESULT"))
    {}

public:
    Wire* CONTROL_EX;
    Wire* RS1V;
    Wire* RS2V;

public:
    Wire* output;
};

class PC_R_Generator: public BaseBlock
{
public:
    static constexpr const char* TypeName = "PC_R_Generator";

public:
    const char* Type() const override
    { return TypeName; }

    void step() override
    {
        bool BRN_COND = INSTRUCTION(*CONTROL_EX).flags.BRN_COND;
        bool CMP_EXIT = this->CMP_EXIT->GetValue<bool>();

        if (BRN_COND && CMP_EXIT)
            *PC_R = true;
        else
            *PC_R = false;
    }

public:
    PC_R_Generator():
        CONTROL_EX (GetWire("CONTROL_EX")), // bits selector?
        CMP_EXIT   (GetWire("CMP RESULT")),
        PC_R       (GetWire("PC_R"))
    {}

public:
    Wire* CONTROL_EX;
    Wire* CMP_EXIT;
    Wire* PC_R;
};

class V_DE_Generator : public BaseBlock
{
public:
    static constexpr const char* TypeName = "V_DE_Generator";

public:
    const char* Type() const override
    { return TypeName; }

    void step() override
    {
        bool PC_RD = this->PC_RD->OldValue<bool>();
        bool PC_RF = this->PC_RF->GetValue<bool>();

        // NOR
        if (PC_RF || PC_RD)
            *V_DE = false;
        else
            *V_DE = true;
    }

public:
    V_DE_Generator():
        PC_RF(GetWire("PC_RF")),
        PC_RD(GetWire("PC_RD")),
        V_DE (GetWire("V_DE"))
    {}

public:
    Wire* PC_RF; // PC_R Fetch
    Wire* PC_RD; // PC_R Decode
    Wire* V_DE;
};

class DataMemory : public BaseBlock
{
public:
    static constexpr const char* TypeName = "DataMemory";

public:
    const char* Type() const override
    { return TypeName; }

    void step() override
    {
        if (*A < size)
        {
            uint32_t extend = INSTRUCTION(*EXTEND).flags.ALUOP;
            bool     sign   = (extend & 0x4);
            uint32_t count  = 1 << (extend & 0x3);

            if (MEM_WE->GetValue<bool>())
            {
                switch(count)
                {
                case 1:
                    memory[*A] = (memory[*A] & 0xffffff00) | (*WD & 0x000000ff);
                    break;
                case 2:
                    memory[*A] = (memory[*A] & 0xffff0000) | (*WD & 0x0000ffff);
                    break;
                case 4:
                    memory[*A] = *WD;
                    break;
                default:
                    throw "DMEM bad count";
                }
            }


            switch(count)
            {
            case 1:
                if (sign)
                    *RD = (memory[*A] & 0x000000ff);
                else
                    *RD = int32_t (memory[*A] & 0x000000ff);
                break;
            case 2:
                if (sign)
                    *RD = (memory[*A] & 0x0000ffff);
                else
                    *RD = int32_t (memory[*A] & 0x0000ffff);
                break;
            case 4:
                *RD = memory[*A];
                break;
            default:
                    throw "DMEM bad count";
            }
        }
        else
            *RD = 0;
    }

public:
    DataMemory():
        EXTEND(GetWire("Memory CONTROL_EX")),
        MEM_WE(GetWire("DMEM WE")),
        WD    (GetWire("DMEM WD")),
        A     (GetWire("DMEM A")),
        RD    (GetWire("DMEM RD"))
    {
        size   = 1000;
        memory = new uint32_t[size];
    }

public:
    Wire* EXTEND; // byte, half, word
    Wire* MEM_WE; // memory write enable
    Wire* WD;     // write data
    Wire* A;      // address

public:
    Wire* RD; // read data

public:
    uint32_t* memory;
    size_t    size;
};

class DMEM_RD_OR_ALU : public BaseBlock
{
public:
    static constexpr const char* TypeName = "DMEM_RD_OR_ALU";

public:
    const char* Type() const override
    { return TypeName; }

    void step() override
    {
        ControlUnitFlags flags = INSTRUCTION(*flag).flags;

        if (flags.MEM2REG)
            *WB_D = *RD;
        else
            *WB_D = *ALU;
    }

public:
    DMEM_RD_OR_ALU():
        flag(GetWire("Memory CONTROL_EX")),
        RD  (GetWire("DMEM RD")),
        ALU (GetWire("Memory ALU")),
        WB_D(GetWire("Memory WB_D"))
    {}

public:
    Wire* flag;
    Wire* RD;
    Wire* ALU;

public:
    Wire* WB_D;
};


void PrintWires()
{
    // Prints output wires of all stages
    std::cout << "-----------------------------------------------------" << std::endl;

    std::cout << "Fetch:" << '\n';
    std::cout << "Fetch instr = 0x" << std::hex << (Wires["IMEM D"]->OldValue()) << std::dec << (INSTRUCTION(Wires["IMEM D"]->OldValue()).opcode() == 0x63 ? " B*": " ADDI") << '\n';
    //std::cout << "Fetch instr = 0x" << std::hex << (Wires["IMEM D"]->OldValue()) << ' ' << INSTRUCTION(Wires["IMEM D"]->OldValue()).opcode() << std::dec << '\n';
    std::cout << "PC_R        = "   << (Wires["PC_R"]->OldValue()) << '\n';
    std::cout << "PC          = "   << (Wires["PC"]->OldValue())   << '\n';
    std::cout << '\n';

    ControlUnitFlags flagsD = INSTRUCTION(Wires["CU FLAGS"]->OldValue()).flags;
    std::cout << "Decode:" << '\n';
    std::cout << "Decode instr = 0x" << std::hex << (Wires["INSTRUCTION"]->OldValue()) << std::dec << (INSTRUCTION(Wires["INSTRUCTION"]->OldValue()).opcode() == 0x63 ? " B*": " ADDI") << '\n';
    std::cout << "PC_DE        = "   << (Wires["PC_DE"]->OldValue()) << '\n';
    std::cout << "RF.RS1       = "   << (Wires["RS1"]->OldValue()) << '\n';
    std::cout << "RF.RS2       = "   << (Wires["RS2"]->OldValue()) << '\n';
    std::cout << "CU.flags     = "   << flagsD.ALUOP << ' ' << flagsD.SRC2 << ' ' << flagsD.BRN_COND << flagsD.MEM2REG << flagsD.MEM_WEN << flagsD.REG_WEN << '\n';
    std::cout << "PC_RF        = "   << (Wires["PC_RF"]->OldValue()) << '\n';
    std::cout << "PC_RD        = "   << (Wires["PC_RD"]->OldValue()) << '\n';
    std::cout << "V_DE         = "   << (Wires["V_DE"]->OldValue()) << '\n';
    std::cout << '\n';

    ControlUnitFlags flagsE = INSTRUCTION(Wires["CONTROL_EX"]->OldValue()).flags;
    std::cout << "Execute:" << '\n';
    std::cout << "Execute instr = 0x" << std::hex << (Wires["Execute INSTRUCTION"]->OldValue()) << std::dec << (INSTRUCTION(Wires["Execute INSTRUCTION"]->OldValue()).opcode() == 0x63 ? " B*": " ADDI") << '\n';
    std::cout << "PC_EX         = "   << (Wires["PC_EX"]->OldValue()) << '\n';
    std::cout << "V_EX          = "   << (Wires["V_EX"]->OldValue())  << '\n';
    std::cout << "WE_GEN WB_WE  = "   << (Wires["WE_GEN WB_WE"]->OldValue())  << '\n';
    std::cout << "WE_GEN MEM_WE = "   << (Wires["WE_GEN MEM_WE"]->OldValue()) << '\n';
    std::cout << "CONTROL_EX    = "   << flagsE.ALUOP << ' ' << flagsE.SRC2 << ' ' << flagsE.REG_WEN << flagsE.MEM_WEN << flagsE.MEM2REG << flagsE.BRN_COND << '\n';
    std::cout << "RF.RS1        = "   << (Wires["Execute RS1"]->OldValue()) << '\n';
    std::cout << "RS1V          = "   << (Wires["RS1V"]->OldValue()) << '\n';
    std::cout << "SRC2          = "   << (Wires["SRC2"]->OldValue()) << '\n';
    std::cout << "ALU           = "   << (Wires["ALU RESULT"]->OldValue()) << '\n';
    std::cout << '\n';

    ControlUnitFlags flagsM = INSTRUCTION(Wires["Memory CONTROL_EX"]->OldValue()).flags;
    std::cout << "Memory:" << '\n';
    std::cout << "Memory instr      = 0x" << std::hex << (Wires["Memory INSTRUCTION"]->OldValue()) << std::dec << (INSTRUCTION(Wires["Memory INSTRUCTION"]->OldValue()).opcode() == 0x63 ? " B*": " ADDI")  << '\n';
    std::cout << "Memory CONTROL_EX = "   << flagsM.ALUOP << ' ' << flagsM.SRC2 << ' ' << flagsM.REG_WEN << flagsM.MEM_WEN << flagsM.MEM2REG << flagsM.BRN_COND << '\n';
    std::cout << "WB_WE             = "   << (Wires["Memory WE_GEN WB_WE"]->OldValue()) << '\n';
    std::cout << "WB_D              = "   << (Wires["Memory WB_D"]->OldValue())         << '\n';
    std::cout << '\n';

    ControlUnitFlags flagsWB = INSTRUCTION(Wires["WB CONTROL_EX"]->OldValue()).flags;
    std::cout << "WB:" << '\n';
    std::cout << "WB instr      = 0x" << std::hex << (Wires["WB_A"]->OldValue()) << std::dec << (INSTRUCTION(Wires["WB_A"]->OldValue()).opcode() == 0x63 ? " B*": " ADDI")  << '\n';
    std::cout << "WB CONTROL_EX = "   << flagsWB.ALUOP << ' ' << flagsWB.SRC2 << ' ' << flagsWB.REG_WEN << flagsWB.MEM_WEN << flagsWB.MEM2REG << flagsWB.BRN_COND << '\n';
    std::cout << "WB_WE         = "   << (Wires["WB_WE"]->OldValue()) << '\n';
    std::cout << "WB_A          = "   << INSTRUCTION(Wires["WB_A"]->OldValue()).r_type.rd << '\n';
    std::cout << "WB_D          = "   << (Wires["WB_D"]->OldValue())  << '\n';

    std::cout << "-----------------------------------------------------" << std::endl;
}

int main()
{
    INSTRUCTION cmds[] = {
        MakeADDI(1, 0, 20),  // r1 = rax (sum)
        MakeADDI(2, 0, 10),  // r2 = rcx (cycle counter)
        MakeBEQ (2, 0, +16), // for(r2 = 10; r2 != 0; --r2)
        MakeADDI(1, 1, 15),  //   r1 += 15;
        MakeADDI(2, 2, -1),
        MakeBEQ (0, 0, -12), // absolute short jump
        MakeADDI(1, 1, 1), // r1 += 1
        MakeADDI(1, 1, 2), // r1 += 2
        MakeADDI(0,  0,  0), // NOP
        MakeADDI(0,  0,  0), // NOP
        MakeADDI(0,  0,  0), // NOP
        MakeADDI(0,  0,  0), // NOP
    };

    FillWires();

    // Stage 1 - Fetch
    InstructionMemory IMEM;
    NextInstruction   NPC;

    IMEM.SetMemory(cmds, sizeof(cmds) / sizeof(cmds[0]));

    // Stage 2 - Decode
    V_DE_Generator  V_DE_GEN;
    ControlUnit     CU;
    RegisterFile    RF;

    // Stage 3 - Execute
    HazardUnit           HU;
    WriteEnableGenerator WE_GEN;
    RS_TO_RSV            RS1V_SEL(1);
    RS_TO_RSV            RS2V_SEL(2);
    Immediate            IMM;
    SRC2_SELECTOR        SRC2_SEL;
    ArithmeticLogicUnit  ALU;

    Comparator     CMP;
    PC_R_Generator PC_R_GEN;

    // Stage 4 - Memory
    DataMemory          DMEM;
    DMEM_RD_OR_ALU      RSEL;

    std::vector<BaseBlock*> STAGE_FETCH   = {
        dynamic_cast<FlipFlop*>(Wires["PC"]),
        &IMEM,
        &NPC,
    };
    std::vector<BaseBlock*> STAGE_DECODE  = {
        dynamic_cast<FlipFlop*>(Wires["INSTRUCTION"]),
        dynamic_cast<FlipFlop*>(Wires["PC_DE"]),
        dynamic_cast<FlipFlop*>(Wires["PC_RF"]),
        &V_DE_GEN,
        &CU,
        &RF,
    };
    std::vector<BaseBlock*> STAGE_EXECUTE = {
        dynamic_cast<FlipFlop*>(Wires["Execute INSTRUCTION"]),
        dynamic_cast<FlipFlop*>(Wires["CONTROL_EX"]),
        dynamic_cast<FlipFlop*>(Wires["Execute RS1"]),
        dynamic_cast<FlipFlop*>(Wires["Execute RS2"]),
        dynamic_cast<FlipFlop*>(Wires["PC_EX"]),
        &HU,
        &WE_GEN,
        &RS1V_SEL,
        &RS2V_SEL,
        &IMM,
        &SRC2_SEL,
        &ALU,
        &CMP,
        &PC_R_GEN,
    };
    std::vector<BaseBlock*> STAGE_MEMORY  = {
        dynamic_cast<FlipFlop*>(Wires["WB_WE"]),
        dynamic_cast<FlipFlop*>(Wires["WB_D"]),
        dynamic_cast<FlipFlop*>(Wires["WB_A"]),

        dynamic_cast<FlipFlop*>(Wires["Memory INSTRUCTION"]),
        dynamic_cast<FlipFlop*>(Wires["Memory WE_GEN MEM_WE"]),
        dynamic_cast<FlipFlop*>(Wires["Memory WE_GEN WB_WE"]),
        dynamic_cast<FlipFlop*>(Wires["Memory CONTROL_EX"]),
        dynamic_cast<FlipFlop*>(Wires["Memory RS1"]),
        dynamic_cast<FlipFlop*>(Wires["Memory ALU"]),

        &DMEM,
        &RSEL,
    };

    // Running
    for(BaseBlock* block : STAGE_FETCH)
        block->step();

    PrintWires();

    ++GLOBAL_STAGE;

    for(BaseBlock* block : STAGE_DECODE)
        block->step();
    for(BaseBlock* block : STAGE_FETCH)
        block->step();

    PrintWires();

    ++GLOBAL_STAGE;

    // we need to do FlipFlop (Fetch -> Decode) for PC_R before execute stage as we recalculate PC_R value
    dynamic_cast<FlipFlop*>(Wires["PC_RF"])->step();

    for(BaseBlock* block : STAGE_EXECUTE)
        block->step();
    for(BaseBlock* block : STAGE_DECODE)
        block->step();
    for(BaseBlock* block : STAGE_FETCH)
        block->step();

    PrintWires();

    ++GLOBAL_STAGE;

    while(true)
    {
        try
        {
            // we need to do FlipFlop (Fetch -> Decode) for PC_R before execute stage as we recalculate PC_R value
            dynamic_cast<FlipFlop*>(Wires["PC_RF"])->step();

            std::cout << "PC_RF = " << Wires["PC_RF"]->value << '\n';
            std::cout << "PC_RD = " << Wires["PC_RD"]->value << '\n';

            for(BaseBlock* block : STAGE_MEMORY)
                block->step();
            for(BaseBlock* block : STAGE_EXECUTE)
            {
                block->step();
                //std::cout << "PC_RF = " << Wires["PC_RF"]->value << '\n';
                //std::cout << "PC_RD = " << Wires["PC_RD"]->value << '\n';
            }
            for(BaseBlock* block : STAGE_DECODE)
            {
                block->step();
                //std::cout << "PC_RF = " << Wires["PC_RF"]->value << '\n';
                //std::cout << "PC_RD = " << Wires["PC_RD"]->value << '\n';
            }
            for(BaseBlock* block : STAGE_FETCH)
                block->step();

            PrintWires();

            ++GLOBAL_STAGE;

            std::cout << "*** r1 = " << RF.regs[1] << std::endl;
            std::cout << "*** r2 = " << RF.regs[2] << std::endl;
        }
        catch(const char* message)
        {
            std::cerr << message << std::endl;
            break;
        }
    }

    return 0;
}
