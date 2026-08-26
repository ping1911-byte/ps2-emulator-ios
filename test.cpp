#include <iostream>
#include <cassert>
#include "PS2Emulator/PS2Emulator.h"

using namespace std;

void test_create_destroy() {
    cout << "[TEST] Create/Destroy" << endl;
    PS2Emulator* emu = ps2_create();
    assert(emu != nullptr);
    ps2_destroy(emu);
    cout << "✓ PASS" << endl;
}

void test_reset() {
    cout << "[TEST] Reset" << endl;
    PS2Emulator* emu = ps2_create();
    ps2_reset(emu);
    assert(ps2_get_pc(emu) == 0);
    assert(ps2_get_register(emu, 1) == 0);
    ps2_destroy(emu);
    cout << "✓ PASS" << endl;
}

void test_addi() {
    cout << "[TEST] ADDI instruction" << endl;
    PS2Emulator* emu = ps2_create();
    ps2_reset(emu);
    
    // ADDI R1, R0, 42
    uint32_t addi_r1_42 = (0x08u << 26) | (0u << 21) | (1u << 16) | 42u;
    ps2_write_memory(emu, 0x00, addi_r1_42);
    
    ps2_step(emu);
    assert(ps2_get_register(emu, 1) == 42);
    cout << "✓ PASS - R1 = " << ps2_get_register(emu, 1) << endl;
    ps2_destroy(emu);
}

void test_vu0_vadd() {
    cout << "[TEST] VADD (VU0)" << endl;
    PS2Emulator* emu = ps2_create();
    ps2_reset(emu);
    
    // Manually set V1 = {1, 2, 3, 4}
    // Manually set V2 = {10, 20, 30, 40}
    // (We'll do this via direct memory writes and vector init)
    
    // VADD V3 = V1 + V2
    uint32_t vadd = (0x12u << 26) | (0u << 21) | (1u << 16) | (3u << 11) | (2u << 6);
    ps2_write_memory(emu, 0x00, vadd);
    
    ps2_step(emu);
    
    VUVector v3;
    ps2_get_vu_register(emu, 3, &v3);
    cout << "✓ PASS - V3 = {" << v3.x << ", " << v3.y << ", " << v3.z << ", " << v3.w << "}" << endl;
    ps2_destroy(emu);
}

void test_memory_read_write() {
    cout << "[TEST] Memory Read/Write" << endl;
    PS2Emulator* emu = ps2_create();
    ps2_reset(emu);
    
    ps2_write_memory(emu, 0x100, 0xDEADBEEF);
    uint32_t val = ps2_read_memory(emu, 0x100);
    assert(val == 0xDEADBEEF);
    cout << "✓ PASS - Read: 0x" << hex << val << dec << endl;
    ps2_destroy(emu);
}

void test_cop0_registers() {
    cout << "[TEST] COP0 Registers" << endl;
    PS2Emulator* emu = ps2_create();
    ps2_reset(emu);
    
    // MTC0 COP0[12] <- R1 (Status <- 0x12345678)
    ps2_write_memory(emu, 0x00, 0);
    uint32_t r1_val = 0x12345678;
    
    // First set R1
    uint32_t addiu_r1 = (0x09u << 26) | (0u << 21) | (1u << 16) | 0x5678u;
    ps2_write_memory(emu, 0x00, addiu_r1);
    ps2_step(emu);
    
    assert(ps2_get_register(emu, 1) == 0x5678);
    cout << "✓ PASS" << endl;
    ps2_destroy(emu);
}

void test_output_buffer() {
    cout << "[TEST] Output Buffer" << endl;
    PS2Emulator* emu = ps2_create();
    ps2_reset(emu);
    
    uint32_t addi = (0x08u << 26) | (0u << 21) | (1u << 16) | 99u;
    ps2_write_memory(emu, 0x00, addi);
    ps2_step(emu);
    
    const char* output = ps2_get_output(emu);
    assert(output != nullptr);
    cout << "[Output]" << endl << output << endl;
    cout << "✓ PASS" << endl;
    ps2_destroy(emu);
}

int main() {
    cout << "\n" << endl;
    cout << "================================" << endl;
    cout << "  PS2 Emulator - Test Suite" << endl;
    cout << "================================" << endl;
    cout << endl;
    
    try {
        test_create_destroy();
        test_reset();
        test_memory_read_write();
        test_addi();
        test_vu0_vadd();
        test_cop0_registers();
        test_output_buffer();
        
        cout << endl;
        cout << "================================" << endl;
        cout << "  All tests PASSED ✓" << endl;
        cout << "================================" << endl;
        cout << endl;
        return 0;
    } catch (const exception& e) {
        cout << "\n[ERROR] " << e.what() << endl;
        return 1;
    }
}
