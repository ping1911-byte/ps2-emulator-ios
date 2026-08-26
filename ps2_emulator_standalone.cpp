#include <iostream>
#include <cstdint>
#include <cstring>
#include <vector>
#include <sstream>

using namespace std;

// ========================================
// PS2 Emulator C++ Standalone
// Runnable on iPhone via C++
// ========================================

uint32_t R[32] = {0};
uint32_t PC = 0;
uint32_t CURRENT_INSTRUCTION_PC = 0;
uint32_t COP0[32] = {0};

struct VUVector {
    float x, y, z, w;
};

VUVector VU0[32] = {};
uint32_t VU0_STATUS = 0;

static constexpr uint32_t MEMORY_SIZE = 32 * 1024 * 1024;
static uint8_t Memory[MEMORY_SIZE] = {0};

static constexpr uint32_t COP0_INDEX_BADVADDR = 8;
static constexpr uint32_t COP0_INDEX_STATUS = 12;
static constexpr uint32_t COP0_INDEX_CAUSE = 13;
static constexpr uint32_t COP0_INDEX_EPC = 14;
static constexpr uint32_t EXCEPTION_VECTOR = 0x00000180;

// Exception codes
static constexpr uint32_t EXC_SYSCALL = 8;
static constexpr uint32_t EXC_BREAK = 9;
static constexpr uint32_t EXC_RI = 10;
static constexpr uint32_t EXC_ADEL = 4;
static constexpr uint32_t EXC_ADES = 5;

// Output buffer
stringstream g_output;

void ps2_reset() {
    memset(R, 0, sizeof(R));
    memset(COP0, 0, sizeof(COP0));
    memset(VU0, 0, sizeof(VU0));
    memset(Memory, 0, MEMORY_SIZE);
    PC = 0;
    CURRENT_INSTRUCTION_PC = 0;
    g_output.str("");
}

void write32(uint32_t address, uint32_t value) {
    if (address + 3 >= MEMORY_SIZE) {
        g_output << "[ERR] Write out of bounds: 0x" << hex << address << dec << endl;
        return;
    }
    Memory[address] = value & 0xFF;
    Memory[address + 1] = (value >> 8) & 0xFF;
    Memory[address + 2] = (value >> 16) & 0xFF;
    Memory[address + 3] = (value >> 24) & 0xFF;
}

uint32_t read32(uint32_t address) {
    if (address + 3 >= MEMORY_SIZE) {
        g_output << "[ERR] Read out of bounds: 0x" << hex << address << dec << endl;
        return 0;
    }
    return
        (uint32_t)Memory[address] |
        ((uint32_t)Memory[address + 1] << 8) |
        ((uint32_t)Memory[address + 2] << 16) |
        ((uint32_t)Memory[address + 3] << 24);
}

static VUVector vuMake(float x, float y, float z, float w) {
    return {x, y, z, w};
}

static VUVector vuAdd(const VUVector& a, const VUVector& b) {
    return vuMake(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

static VUVector vuMul(const VUVector& a, const VUVector& b) {
    return vuMake(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
}

static VUVector vuMadd(const VUVector& a, const VUVector& b, const VUVector& c) {
    return vuMake(a.x * b.x + c.x, a.y * b.y + c.y, a.z * b.z + c.z, a.w * b.w + c.w);
}

static float vuDot(const VUVector& a, const VUVector& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

void raiseException(uint32_t code, uint32_t badAddress = 0) {
    COP0[COP0_INDEX_CAUSE] &= ~(0x1Fu << 2);
    COP0[COP0_INDEX_CAUSE] |= (code & 0x1F) << 2;
    if (code == EXC_ADEL || code == EXC_ADES)
        COP0[COP0_INDEX_BADVADDR] = badAddress;
    COP0[COP0_INDEX_EPC] = CURRENT_INSTRUCTION_PC;
    COP0[COP0_INDEX_STATUS] |= (1u << 1);
    PC = EXCEPTION_VECTOR;
    g_output << "[EXCEPTION] code=" << code << " EPC=0x" << hex << COP0[COP0_INDEX_EPC] << dec << endl;
}

void executeVUInstruction(uint32_t instruction) {
    uint32_t rs = (instruction >> 21) & 0x1F;
    uint32_t rt = (instruction >> 16) & 0x1F;
    uint32_t rd = (instruction >> 11) & 0x1F;
    uint32_t shamt = (instruction >> 6) & 0x1F;
    
    switch (rs) {
        case 0:
            VU0[rd] = vuAdd(VU0[rt], VU0[shamt]);
            g_output << "  VADD V" << rd << " = V" << rt << " + V" << shamt << endl;
            break;
        case 1:
            VU0[rd] = vuMul(VU0[rt], VU0[shamt]);
            g_output << "  VMUL V" << rd << " = V" << rt << " * V" << shamt << endl;
            break;
        case 2:
            VU0[rd] = vuMadd(VU0[rt], VU0[shamt], VU0[rd]);
            g_output << "  VMADD V" << rd << " = V" << rt << " * V" << shamt << " + V" << rd << endl;
            break;
        case 3: {
            float result = vuDot(VU0[rt], VU0[shamt]);
            VU0[rd] = vuMake(result, result, result, result);
            g_output << "  VDOT V" << rd << " = dot(V" << rt << ", V" << shamt << ")" << endl;
            break;
        }
        default:
            g_output << "  Unknown VU0 op: " << rs << endl;
            break;
    }
}

void execute(uint32_t instruction) {
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
                    R[rd] = R[rt] << shamt;
                    g_output << "  SLL R" << rd << endl;
                    break;
                case 0x02:
                    R[rd] = R[rt] >> shamt;
                    g_output << "  SRL R" << rd << endl;
                    break;
                case 0x20:
                    R[rd] = static_cast<uint32_t>(static_cast<int32_t>(R[rs]) + static_cast<int32_t>(R[rt]));
                    g_output << "  ADD R" << rd << endl;
                    break;
                case 0x21:
                    R[rd] = R[rs] + R[rt];
                    g_output << "  ADDU R" << rd << endl;
                    break;
                case 0x24:
                    R[rd] = R[rs] & R[rt];
                    g_output << "  AND R" << rd << endl;
                    break;
                case 0x25:
                    R[rd] = R[rs] | R[rt];
                    g_output << "  OR R" << rd << endl;
                    break;
                default:
                    g_output << "  Unknown funct: 0x" << hex << funct << dec << endl;
                    raiseException(EXC_RI);
                    break;
            }
            break;
        case 0x08:
            R[rt] = static_cast<uint32_t>(static_cast<int32_t>(R[rs]) + simm);
            g_output << "  ADDI R" << rt << " <- R" << rs << " + " << simm << endl;
            break;
        case 0x09:
            R[rt] = R[rs] + static_cast<uint32_t>(simm);
            g_output << "  ADDIU R" << rt << endl;
            break;
        case 0x0C:
            R[rt] = R[rs] & uimm;
            g_output << "  ANDI R" << rt << endl;
            break;
        case 0x0D:
            R[rt] = R[rs] | uimm;
            g_output << "  ORI R" << rt << endl;
            break;
        case 0x0F:
            R[rt] = uimm << 16;
            g_output << "  LUI R" << rt << endl;
            break;
        case 0x10:
            if (rs == 0x00) {
                R[rt] = COP0[rd];
                g_output << "  MFC0 R" << rt << " <- COP0[" << rd << "]" << endl;
            } else if (rs == 0x04) {
                COP0[rd] = R[rt];
                g_output << "  MTC0 COP0[" << rd << "] <- R" << rt << endl;
            } else if (rs == 0x10 && funct == 0x18) {
                PC = COP0[COP0_INDEX_EPC];
                COP0[COP0_INDEX_STATUS] &= ~(1u << 1);
                g_output << "  ERET" << endl;
            } else {
                raiseException(EXC_RI);
            }
            break;
        case 0x12:
            executeVUInstruction(instruction);
            break;
        case 0x23: {
            uint32_t address = R[rs] + static_cast<uint32_t>(simm);
            if (address & 3u) {
                raiseException(EXC_ADEL, address);
            } else if (address <= MEMORY_SIZE - 4) {
                R[rt] = read32(address);
                g_output << "  LW R" << rt << " <- [0x" << hex << address << dec << "]" << endl;
            } else {
                raiseException(EXC_ADEL, address);
            }
            break;
        }
        case 0x2B: {
            uint32_t address = R[rs] + static_cast<uint32_t>(simm);
            if (address & 3u) {
                raiseException(EXC_ADES, address);
            } else if (address <= MEMORY_SIZE - 4) {
                write32(address, R[rt]);
                g_output << "  SW [0x" << hex << address << dec << "] <- R" << rt << endl;
            } else {
                raiseException(EXC_ADES, address);
            }
            break;
        }
        default:
            g_output << "  Unknown opcode: 0x" << hex << opcode << dec << endl;
            raiseException(EXC_RI);
            break;
    }
    
    R[0] = 0;
}

bool cpuStep() {
    if (PC > MEMORY_SIZE - 4) {
        g_output << "[ERR] PC outside RAM" << endl;
        return false;
    }
    
    CURRENT_INSTRUCTION_PC = PC;
    uint32_t instruction = read32(PC);
    PC += 4;
    
    g_output << "PC=0x" << hex << CURRENT_INSTRUCTION_PC << dec << " Instr=0x" << hex << instruction << dec << endl;
    execute(instruction);
    
    return true;
}

// ========================================
// Main Demo
// ========================================

int main() {
    cout << "\n================================" << endl;
    cout << "  PS2 Emulator Phase 11" << endl;
    cout << "  iPhone C++ Standalone" << endl;
    cout << "================================\n" << endl;
    
    ps2_reset();
    
    // Initialize VU0 registers
    VU0[1] = vuMake(1.0f, 2.0f, 3.0f, 4.0f);
    VU0[2] = vuMake(10.0f, 20.0f, 30.0f, 40.0f);
    
    cout << "[INIT] V1 = {1, 2, 3, 4}" << endl;
    cout << "[INIT] V2 = {10, 20, 30, 40}" << endl;
    cout << endl;
    
    // Load demo program
    auto makeVUInstruction = [](uint32_t op, uint32_t rt, uint32_t rd, uint32_t second) {
        return (0x12u << 26) | ((op & 0x1Fu) << 21) | ((rt & 0x1Fu) << 16) | ((rd & 0x1Fu) << 11) | ((second & 0x1Fu) << 6);
    };
    
    write32(0x00, makeVUInstruction(0, 1, 3, 2)); // VADD V3 = V1 + V2
    write32(0x04, makeVUInstruction(1, 1, 4, 2)); // VMUL V4 = V1 * V2
    write32(0x08, makeVUInstruction(3, 1, 5, 2)); // VDOT V5 = dot(V1, V2)
    write32(0x0C, makeVUInstruction(2, 1, 6, 2)); // VMADD V6 = V1 * V2 + V6
    
    cout << "[DEMO] Program loaded at 0x00\n" << endl;
    
    // Execute
    for (int i = 0; i < 4; ++i) {
        cout << "Cycle " << (i + 1) << endl;
        if (!cpuStep()) break;
    }
    
    // Results
    cout << "\n================================" << endl;
    cout << "  Results" << endl;
    cout << "================================" << endl;
    cout << "V3 = {" << VU0[3].x << ", " << VU0[3].y << ", " << VU0[3].z << ", " << VU0[3].w << "}" << endl;
    cout << "V4 = {" << VU0[4].x << ", " << VU0[4].y << ", " << VU0[4].z << ", " << VU0[4].w << "}" << endl;
    cout << "V5 = {" << VU0[5].x << ", " << VU0[5].y << ", " << VU0[5].z << ", " << VU0[5].w << "}" << endl;
    cout << "V6 = {" << VU0[6].x << ", " << VU0[6].y << ", " << VU0[6].z << ", " << VU0[6].w << "}" << endl;
    
    cout << "\n[Output]" << endl;
    cout << g_output.str();
    
    cout << "\n================================" << endl;
    cout << "  Complete!" << endl;
    cout << "================================\n" << endl;
    
    return 0;
}
