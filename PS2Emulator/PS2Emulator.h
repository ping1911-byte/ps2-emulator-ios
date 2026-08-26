#ifndef PS2EMULATOR_H
#define PS2EMULATOR_H

#include <cstdint>
#include <string>
#include <vector>

// ========================================
// PS2 Emulator C Interface for iOS
// ========================================

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle to emulator instance
typedef struct PS2Emulator PS2Emulator;

// Vector register representation
typedef struct {
    float x, y, z, w;
} VUVector;

// Create emulator instance
PS2Emulator* ps2_create(void);

// Destroy emulator instance
void ps2_destroy(PS2Emulator* emu);

// Reset emulator state
void ps2_reset(PS2Emulator* emu);

// Load ELF file
int ps2_load_elf(PS2Emulator* emu, const char* filename);

// Execute one CPU step
int ps2_step(PS2Emulator* emu);

// Get current PC
uint32_t ps2_get_pc(PS2Emulator* emu);

// Get general register
uint32_t ps2_get_register(PS2Emulator* emu, int index);

// Get VU0 vector register
void ps2_get_vu_register(PS2Emulator* emu, int index, VUVector* out);

// Read memory
uint32_t ps2_read_memory(PS2Emulator* emu, uint32_t address);

// Write memory
void ps2_write_memory(PS2Emulator* emu, uint32_t address, uint32_t value);

// Get console output
const char* ps2_get_output(PS2Emulator* emu);

// Clear console output
void ps2_clear_output(PS2Emulator* emu);

// Get COP0 register
uint32_t ps2_get_cop0(PS2Emulator* emu, int index);

#ifdef __cplusplus
}
#endif

#endif // PS2EMULATOR_H
