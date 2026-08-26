#include "PS2Emulator.h"
#include <iostream>
#include <cstdint>
#include <fstream>
#include <vector>
#include <cstring>
#include <iomanip>
#include <string>
#include <sstream>

using namespace std;

// ========================================
// PS2 Emulator Instance
// ========================================

struct PS2Emulator {
    uint32_t R[32];
    uint32_t PC;
    uint32_t CURRENT_INSTRUCTION_PC;
    uint32_t COP0[32];
    
    struct VUVectorInternal {
        float x, y, z, w;
    };
    VUVectorInternal VU0[32];
    uint32_t VU0_STATUS;
    
    uint8_t* Memory;
    uint32_t MEMORY_SIZE;
    
    ostringstream output;
    
    PS2Emulator() : PC(0), CURRENT_INSTRUCTION_PC(0), VU0_STATUS(0), MEMORY_SIZE(32 * 1024 * 1024) {
        Memory = new uint8_t[MEMORY_SIZE]();
        memset(R, 0, sizeof(R));
        memset(COP0, 0, sizeof(COP0));
        memset(VU0, 0, sizeof(VU0));
    }
    
    ~PS2Emulator() {
        delete[] Memory;
    }
};

static constexpr uint32_t COP0_INDEX_BADVADDR = 8;
static constexpr uint32_t COP0_INDEX_STATUS = 12;
static constexpr uint32_t COP0_INDEX_CAUSE = 13;
static constexpr uint32_t COP0_INDEX_EPC = 14;
static constexpr uint32_t EXCEPTION_VECTOR = 0x00000180;
static constexpr uint32_t EXC_SYSCALL = 8;
static constexpr uint32_t EXC_BREAK = 9;
static constexpr uint32_t EXC_RI = 10;
static constexpr uint32_t EXC_ADEL = 4;
static constexpr uint32_t EXC_ADES = 5;

PS2Emulator* ps2_create(void) {
    return new PS2Emulator();
}

void ps2_destroy(PS2Emulator* emu) {
    delete emu;
}

void ps2_reset(PS2Emulator* emu) {
    memset(emu->R, 0, sizeof(emu->R));
    memset(emu->COP0, 0, sizeof(emu->COP0));
    memset(emu->VU0, 0, sizeof(emu->VU0));
    memset(emu->Memory, 0, emu->MEMORY_SIZE);
    emu->PC = 0;
    emu->CURRENT_INSTRUCTION_PC = 0;
    emu->output.str("");
}

uint32_t ps2_read_memory_internal(PS2Emulator* emu, uint32_t address) {
    if (address + 3 >= emu->MEMORY_SIZE) {
        emu->output << "[ERR] Memory read out of bounds: 0x" << hex << address << dec << endl;
        return 0;
    }
    return
        (uint32_t)emu->Memory[address] |
        ((uint32_t)emu->Memory[address + 1] << 8) |
        ((uint32_t)emu->Memory[address + 2] << 16) |
        ((uint32_t)emu->Memory[address + 3] << 24);
}

void ps2_write_memory_internal(PS2Emulator* emu, uint32_t address, uint32_t value) {
    if (address + 3 >= emu->MEMORY_SIZE) {
        emu->output << "[ERR] Memory write out of bounds: 0x" << hex << address << dec << endl;
        return;
    }
    emu->Memory[address] = value & 0xFF;
    emu->Memory[address + 1] = (value >> 8) & 0xFF;
    emu->Memory[address + 2] = (value >> 16) & 0xFF;
    emu->Memory[address + 3] = (value >> 24) & 0xFF;
}

void ps2_raise_exception(PS2Emulator* emu, uint32_t code, uint32_t badAddress = 0) {
    emu->COP0[COP0_INDEX_CAUSE] &= ~(0x1Fu << 2);
    emu->COP0[COP0_INDEX_CAUSE] |= (code & 0x1F) << 2;
    if (code == EXC_ADEL || code == EXC_ADES)
        emu->COP0[COP0_INDEX_BADVADDR] = badAddress;
    emu->COP0[COP0_INDEX_EPC] = emu->CURRENT_INSTRUCTION_PC;
    emu->COP0[COP0_INDEX_STATUS] |= (1u << 1);
    emu->PC = EXCEPTION_VECTOR;
    emu->output << "[EXCEPTION] code=" << code << " PC=0x" << hex << emu->COP0[COP0_INDEX_EPC] << dec << endl;
}

PS2Emulator::VUVectorInternal vuMake(float x, float y, float z, float w) {
    return {x, y, z, w};
}

PS2Emulator::VUVectorInternal vuAdd(const PS2Emulator::VUVectorInternal& a, const PS2Emulator::VUVectorInternal& b) {
    return vuMake(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

PS2Emulator::VUVectorInternal vuMul(const PS2Emulator::VUVectorInternal& a, const PS2Emulator::VUVectorInternal& b) {
    return vuMake(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
}

PS2Emulator::VUVectorInternal vuMadd(const PS2Emulator::VUVectorInternal& a, const PS2Emulator::VUVectorInternal& b, const PS2Emulator::VUVectorInternal& c) {
    return vuMake(a.x * b.x + c.x, a.y * b.y + c.y, a.z * b.z + c.z, a.w * b.w + c.w);
}

float vuDot(const PS2Emulator::VUVectorInternal& a, const PS2Emulator::VUVectorInternal& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

void ps2_execute_vu_instruction(PS2Emulator* emu, uint32_t instruction) {
    uint32_t rs = (instruction >> 21) & 0x1F;
    uint32_t rt = (instruction >> 16) & 0x1F;
    uint32_t rd = (instruction >> 11) & 0x1F;
    uint32_t shamt = (instruction >> 6) & 0x1F;
    
    switch (rs) {
        case 0:
            emu->VU0[rd] = vuAdd(emu->VU0[rt], emu->VU0[shamt]);
            emu->output << "  VADD V" << rd << " = V" << rt << " + V" << shamt << endl;
            break;
        case 1:
            emu->VU0[rd] = vuMul(emu->VU0[rt], emu->VU0[shamt]);
            emu->output << "  VMUL V" << rd << " = V" << rt << " * V" << shamt << endl;
            break;
        case 2:
            emu->VU0[rd] = vuMadd(emu->VU0[rt], emu->VU0[shamt], emu->VU0[rd]);
            emu->output << "  VMADD V" << rd << " = V" << rt << " * V" << shamt << " + V" << rd << endl;
            break;
        case 3: {
            float result = vuDot(emu->VU0[rt], emu->VU0[shamt]);
            emu->VU0[rd] = vuMake(result, result, result, result);
            emu->output << "  VDOT V" << rd << " = dot(V" << rt << ", V" << shamt << ")" << endl;
            break;
        }
        default:
            emu->output << "  Unknown VU0 op: " << rs << endl;
            break;
    }
}

void ps2_execute(PS2Emulator* emu, uint32_t instruction) {
    uint32_t opcode = (instruction >> 26) & 0x3F;
    uint32_t rs = (instruction >> 21) & 0x1F;
    uint32_t rt = (instruction >> 16) & 0x1F;
    uint32_t rd = (instruction >> 11) & 0x1F;
    uint32_t shamt = (instruction >> 6) & 0x1F;
    uint32_t funct = instruction & 0x3F;
    int32_t simm = static_cast<int16_t>(instruction & 0xFFFF);
    uint32_t uimm = instruction & 0xFFFF;
    
    switch (opcode) {
        case 0x00:
            switch (funct) {
                case 0x00:
                    emu->R[rd] = emu->R[rt] << shamt;
                    emu->output << "  SLL R" << rd << endl;
                    break;
                case 0x02:
                    emu->R[rd] = emu->R[rt] >> shamt;
                    emu->output << "  SRL R" << rd << endl;
                    break;
                case 0x03:
                    emu->R[rd] = static_cast<uint32_t>(static_cast<int32_t>(emu->R[rt]) >> shamt);
                    emu->output << "  SRA R" << rd << endl;
                    break;
                case 0x08:
                    emu->PC = emu->R[rs];
                    emu->output << "  JR -> 0x" << hex << emu->PC << dec << endl;
                    break;
                case 0x09:
                    emu->R[rd] = emu->PC;
                    emu->PC = emu->R[rs];
                    emu->output << "  JALR R" << rd << endl;
                    break;
                case 0x0C:
                    emu->output << "  SYSCALL" << endl;
                    ps2_raise_exception(emu, EXC_SYSCALL);
                    break;
                case 0x0D:
                    emu->output << "  BREAK" << endl;
                    ps2_raise_exception(emu, EXC_BREAK);
                    break;
                case 0x20:
                    emu->R[rd] = static_cast<uint32_t>(static_cast<int32_t>(emu->R[rs]) + static_cast<int32_t>(emu->R[rt]));
                    emu->output << "  ADD R" << rd << endl;
                    break;
                case 0x21:
                    emu->R[rd] = emu->R[rs] + emu->R[rt];
                    emu->output << "  ADDU R" << rd << endl;
                    break;
                case 0x24:
                    emu->R[rd] = emu->R[rs] & emu->R[rt];
                    emu->output << "  AND R" << rd << endl;
                    break;
                case 0x25:
                    emu->R[rd] = emu->R[rs] | emu->R[rt];
                    emu->output << "  OR R" << rd << endl;
                    break;
                default:
                    emu->output << "  Unknown funct: 0x" << hex << funct << dec << endl;
                    ps2_raise_exception(emu, EXC_RI);
                    break;
            }
            break;
        case 0x08:
            emu->R[rt] = static_cast<uint32_t>(static_cast<int32_t>(emu->R[rs]) + simm);
            emu->output << "  ADDI R" << rt << endl;
            break;
        case 0x09:
            emu->R[rt] = emu->R[rs] + static_cast<uint32_t>(simm);
            emu->output << "  ADDIU R" << rt << endl;
            break;
        case 0x0C:
            emu->R[rt] = emu->R[rs] & uimm;
            emu->output << "  ANDI R" << rt << endl;
            break;
        case 0x0D:
            emu->R[rt] = emu->R[rs] | uimm;
            emu->output << "  ORI R" << rt << endl;
            break;
        case 0x0F:
            emu->R[rt] = uimm << 16;
            emu->output << "  LUI R" << rt << endl;
            break;
        case 0x10:
            if (rs == 0x00) {
                emu->R[rt] = emu->COP0[rd];
                emu->output << "  MFC0 R" << rt << " <- COP0[" << rd << "]" << endl;
            } else if (rs == 0x04) {
                emu->COP0[rd] = emu->R[rt];
                emu->output << "  MTC0 COP0[" << rd << "] <- R" << rt << endl;
            } else if (rs == 0x10 && funct == 0x18) {
                emu->PC = emu->COP0[COP0_INDEX_EPC];
                emu->COP0[COP0_INDEX_STATUS] &= ~(1u << 1);
                emu->output << "  ERET" << endl;
            } else {
                ps2_raise_exception(emu, EXC_RI);
            }
            break;
        case 0x12:
            ps2_execute_vu_instruction(emu, instruction);
            break;
        case 0x23: {
            uint32_t address = emu->R[rs] + static_cast<uint32_t>(simm);
            if (address & 3u) {
                ps2_raise_exception(emu, EXC_ADEL, address);
            } else if (address <= emu->MEMORY_SIZE - 4) {
                emu->R[rt] = ps2_read_memory_internal(emu, address);
                emu->output << "  LW R" << rt << " <- [0x" << hex << address << dec << "]" << endl;
            } else {
                ps2_raise_exception(emu, EXC_ADEL, address);
            }
            break;
        }
        case 0x2B: {
            uint32_t address = emu->R[rs] + static_cast<uint32_t>(simm);
            if (address & 3u) {
                ps2_raise_exception(emu, EXC_ADES, address);
            } else if (address <= emu->MEMORY_SIZE - 4) {
                ps2_write_memory_internal(emu, address, emu->R[rt]);
                emu->output << "  SW [0x" << hex << address << dec << "] <- R" << rt << endl;
            } else {
                ps2_raise_exception(emu, EXC_ADES, address);
            }
            break;
        }
        default:
            emu->output << "  Unknown opcode: 0x" << hex << opcode << dec << endl;
            ps2_raise_exception(emu, EXC_RI);
            break;
    }
    
    emu->R[0] = 0;
}

int ps2_step(PS2Emulator* emu) {
    if (emu->PC > emu->MEMORY_SIZE - 4) {
        emu->output << "[ERR] PC outside RAM: 0x" << hex << emu->PC << dec << endl;
        return 0;
    }
    
    emu->CURRENT_INSTRUCTION_PC = emu->PC;
    uint32_t instruction = ps2_read_memory_internal(emu, emu->PC);
    emu->PC += 4;
    
    emu->output << "PC=0x" << hex << emu->CURRENT_INSTRUCTION_PC << dec << " Instr=0x" << hex << instruction << dec << endl;
    ps2_execute(emu, instruction);
    
    return 1;
}

uint32_t ps2_get_pc(PS2Emulator* emu) {
    return emu->PC;
}

uint32_t ps2_get_register(PS2Emulator* emu, int index) {
    if (index >= 0 && index < 32) return emu->R[index];
    return 0;
}

void ps2_get_vu_register(PS2Emulator* emu, int index, VUVector* out) {
    if (index >= 0 && index < 32) {
        out->x = emu->VU0[index].x;
        out->y = emu->VU0[index].y;
        out->z = emu->VU0[index].z;
        out->w = emu->VU0[index].w;
    }
}

uint32_t ps2_read_memory(PS2Emulator* emu, uint32_t address) {
    return ps2_read_memory_internal(emu, address);
}

void ps2_write_memory(PS2Emulator* emu, uint32_t address, uint32_t value) {
    ps2_write_memory_internal(emu, address, value);
}

const char* ps2_get_output(PS2Emulator* emu) {
    return emu->output.str().c_str();
}

void ps2_clear_output(PS2Emulator* emu) {
    emu->output.str("");
}

uint32_t ps2_get_cop0(PS2Emulator* emu, int index) {
    if (index >= 0 && index < 32) return emu->COP0[index];
    return 0;
}

int ps2_load_elf(PS2Emulator* emu, const char* filename) {
    ifstream file(filename, ios::binary);
    if (!file) return 0;
    
    file.seekg(0, ios::end);
    streamoff size = file.tellg();
    file.seekg(0, ios::beg);
    
    if (size < 52) return 0;
    
    vector<uint8_t> data(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(data.data()), size)) return 0;
    
    if (data[0] != 0x7F || data[1] != 'E' || data[2] != 'L' || data[3] != 'F') return 0;
    
    emu->output << "[ELF] Loaded successfully" << endl;
    return 1;
}
