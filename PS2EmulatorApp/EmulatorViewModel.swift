import Foundation

class EmulatorViewModel: ObservableObject {
    private var emulator: OpaquePointer?
    
    @Published var output: String = ""
    @Published var pc: UInt32 = 0
    @Published var registers: [UInt32] = Array(repeating: 0, count: 32)
    @Published var cop0: [UInt32] = Array(repeating: 0, count: 32)
    @Published var vuRegisters: [(Float, Float, Float, Float)] = Array(repeating: (0, 0, 0, 0), count: 32)
    @Published var memoryValue: UInt32? = nil
    
    init() {
        emulator = ps2_create()
        reset()
    }
    
    deinit {
        if let emu = emulator {
            ps2_destroy(emu)
        }
    }
    
    func reset() {
        guard let emu = emulator else { return }
        ps2_reset(emu)
        updateState()
        
        // Load demo program
        loadDemoProgram()
    }
    
    func step() {
        guard let emu = emulator else { return }
        ps2_step(emu)
        updateState()
    }
    
    func runSteps(_ count: Int) {
        for _ in 0..<count {
            step()
        }
    }
    
    func readMemory(_ address: UInt32) {
        guard let emu = emulator else { return }
        memoryValue = ps2_read_memory(emu, address)
    }
    
    private func updateState() {
        guard let emu = emulator else { return }
        
        pc = ps2_get_pc(emu)
        
        for i in 0..<32 {
            registers[i] = ps2_get_register(emu, Int32(i))
        }
        
        for i in 0..<32 {
            var vec = VUVector(x: 0, y: 0, z: 0, w: 0)
            ps2_get_vu_register(emu, Int32(i), &vec)
            vuRegisters[i] = (vec.x, vec.y, vec.z, vec.w)
        }
        
        for i in 0..<32 {
            cop0[i] = ps2_get_cop0(emu, Int32(i))
        }
        
        output = String(cString: ps2_get_output(emu))
    }
    
    private func loadDemoProgram() {
        guard let emu = emulator else { return }
        
        // Initialize VU0 registers
        // V1 = {1, 2, 3, 4}
        // V2 = {10, 20, 30, 40}
        ps2_write_memory(emu, 0x00, makeVUInstruction(0, 1, 3, 2)) // VADD V3 = V1 + V2
        ps2_write_memory(emu, 0x04, makeVUInstruction(1, 1, 4, 2)) // VMUL V4 = V1 * V2
        ps2_write_memory(emu, 0x08, makeVUInstruction(3, 1, 5, 2)) // VDOT V5 = dot(V1, V2)
        ps2_write_memory(emu, 0x0C, makeVUInstruction(2, 1, 6, 2)) // VMADD V6 = V1 * V2 + V6
        
        output = "[Demo] VU0 program loaded at 0x00\n"
        updateState()
    }
    
    private func makeVUInstruction(_ operation: UInt32, _ rt: UInt32, _ rd: UInt32, _ second: UInt32) -> UInt32 {
        return (0x12 << 26) |
               ((operation & 0x1F) << 21) |
               ((rt & 0x1F) << 16) |
               ((rd & 0x1F) << 11) |
               ((second & 0x1F) << 6)
    }
}
