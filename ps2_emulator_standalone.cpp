#include <iostream>
#include <cstdint>
#include <fstream>
#include <vector>
#include <cstring>
#include <iomanip>
#include <string>

using namespace std;

// ========================================
// PS2 Emulator Prototype
// Phase 9: MIPS R5900 CPU
// ========================================

uint32_t R[32] = {0};
uint32_t PC = 0;
uint32_t CURRENT_INSTRUCTION_PC = 0;

// ========================================
// COP0 - System Control Registers
// ========================================
// Indexes follow the MIPS COP0 register numbers.
uint32_t COP0[32] = {0};

// ========================================
// COP2 / VU0 Prototype
// ========================================
// PS2 has vector units with 128-bit vector registers.
// This prototype models 32 VU0 vector registers, each with
// four single-precision components: X, Y, Z, W.
struct VUVector {
    float x;
    float y;
    float z;
    float w;
};

VUVector VU0[32] = {};

uint32_t VU0_STATUS = 0;

static VUVector vuMake(float x, float y, float z, float w) {
    VUVector v{x, y, z, w};
    return v;
}

static void vuPrint(const char* name, uint32_t reg) {
    cout << name << reg << " = { "
         << VU0[reg].x << ", "
         << VU0[reg].y << ", "
         << VU0[reg].z << ", "
         << VU0[reg].w << " }"
         << endl;
}

static VUVector vuAdd(const VUVector& a, const VUVector& b) {
    return vuMake(
        a.x + b.x,
        a.y + b.y,
        a.z + b.z,
        a.w + b.w
    );
}

static VUVector vuMul(const VUVector& a, const VUVector& b) {
    return vuMake(
        a.x * b.x,
        a.y * b.y,
        a.z * b.z,
        a.w * b.w
    );
}

static VUVector vuMadd(const VUVector& a,
                       const VUVector& b,
                       const VUVector& c) {
    return vuMake(
        a.x * b.x + c.x,
        a.y * b.y + c.y,
        a.z * b.z + c.z,
        a.w * b.w + c.w
    );
}

static float vuDot(const VUVector& a, const VUVector& b) {
    return a.x * b.x +
           a.y * b.y +
           a.z * b.z +
           a.w * b.w;
}


static constexpr uint32_t COP0_INDEX_BADVADDR = 8;
static constexpr uint32_t COP0_INDEX_STATUS   = 12;
static constexpr uint32_t COP0_INDEX_CAUSE    = 13;
static constexpr uint32_t COP0_INDEX_EPC      = 14;

// Prototype exception vector.
// Real PS2/MIPS uses KSEG0-style virtual addresses; this prototype
// keeps the vector inside the 32 MB flat RAM address space.
static constexpr uint32_t EXCEPTION_VECTOR = 0x00000180;

// Exception codes used by this phase.
static constexpr uint32_t EXC_SYSCALL      = 8;
static constexpr uint32_t EXC_BREAK        = 9;
static constexpr uint32_t EXC_RI           = 10;
static constexpr uint32_t EXC_ADEL         = 4;
static constexpr uint32_t EXC_ADES         = 5;

static constexpr uint32_t MEMORY_SIZE = 32 * 1024 * 1024;
uint8_t Memory[MEMORY_SIZE] = {0};


// ========================================
// Phase 13 - EE INTC + Timer
// ========================================

static constexpr uint32_t INTC_BASE = 0x1000F000;
static constexpr uint32_t TIMER0_BASE = 0x10001000;

static constexpr uint32_t INTC_STAT = INTC_BASE + 0x00;
static constexpr uint32_t INTC_MASK = INTC_BASE + 0x04;

static constexpr uint32_t T0_COUNT = TIMER0_BASE + 0x00;
static constexpr uint32_t T0_MODE  = TIMER0_BASE + 0x04;
static constexpr uint32_t T0_COMP  = TIMER0_BASE + 0x08;

static constexpr uint32_t INTC_TIMER0 = (1u << 9);
static constexpr uint32_t COP0_STATUS_IE  = (1u << 0);
static constexpr uint32_t COP0_STATUS_EXL = (1u << 1);
static constexpr uint32_t COP0_CAUSE_IP2  = (1u << 10);

static uint32_t INTC_STAT_REG = 0;
static uint32_t INTC_MASK_REG = 0;
static uint32_t TIMER0_COUNT_REG = 0;
static uint32_t TIMER0_MODE_REG = 0;
static uint32_t TIMER0_COMP_REG = 0;

static void timer0Reset() {
    TIMER0_COUNT_REG = 0;
    TIMER0_MODE_REG = 0;
    TIMER0_COMP_REG = 0;
    INTC_STAT_REG = 0;
    INTC_MASK_REG = 0;
}

static void intcRaise(uint32_t source) {
    INTC_STAT_REG |= source;
    cout << "[INTC] IRQ raised: 0x" << hex << source << dec << endl;
}

static bool interruptPending() {
    return (INTC_STAT_REG & INTC_MASK_REG) != 0 &&
           (COP0[COP0_INDEX_STATUS] & COP0_STATUS_IE) != 0 &&
           (COP0[COP0_INDEX_STATUS] & COP0_STATUS_EXL) == 0;
}

static void timer0Step(uint32_t cycles = 1) {
    if ((TIMER0_MODE_REG & 1u) == 0 || TIMER0_COMP_REG == 0)
        return;

    for (uint32_t i = 0; i < cycles; ++i) {
        ++TIMER0_COUNT_REG;
        if (TIMER0_COUNT_REG >= TIMER0_COMP_REG) {
            TIMER0_COUNT_REG = 0;
            intcRaise(INTC_TIMER0);
        }
    }
}

// Phase 14 interrupt identifiers are declared early because
// the shared COP0 interrupt service routine acknowledges DMA too.
static constexpr uint32_t DMAC_STAT_GIF = (1u << 2);
static constexpr uint32_t INTC_DMA = (1u << 10);
static constexpr uint32_t INTC_VIF0 = (1u << 7);
static constexpr uint32_t INTC_VIF1 = (1u << 8);
static constexpr uint32_t DMAC_STAT_VIF0 = (1u << 0);
static constexpr uint32_t DMAC_STAT_VIF1 = (1u << 1);
static uint32_t DMAC_D_STAT_REG = 0;

