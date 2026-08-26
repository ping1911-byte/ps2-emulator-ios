# PS2 Emulator Phase 11 - iOS App

A PlayStation 2 MIPS R5900 CPU emulator with VU0 (Vector Unit) support, running as an interactive iOS application.

## Features

✅ **MIPS R5900 CPU Emulation**
- 32 general-purpose registers
- Full instruction set (SPECIAL, I-type, J-type, load/store)
- PC-relative branching (BEQ, BNE, BLEZ, BGTZ)
- Jump and link (JAL, JALR)

✅ **COP0 - System Control**
- Exception handling (SYSCALL, BREAK, RI, ADEL, ADES)
- Status, Cause, EPC, BadVAddr registers
- Exception vector dispatch

✅ **COP2 / VU0 - Vector Unit**
- 32 × 128-bit vector registers (4-wide float32)
- VADD (vector add)
- VMUL (element-wise multiply)
- VMADD (multiply-accumulate)
- VDOT (dot product)

✅ **Memory System**
- 32 MB flat address space
- 32-bit word-aligned load/store (LW, SW)
- Alignment exception handling
- Bounds checking

✅ **iOS Interface**
- Real-time console output
- Register viewer (R0-R31, COP0)
- VU0 vector visualization
- Memory inspector
- Step-by-step execution
- Batch execution (10x)

## Architecture

```
PS2Emulator.h         ← C interface for iOS
    ↓
PS2Emulator.cpp       ← Core emulator (MIPS, COP0, VU0)
    ↓
EmulatorViewModel.swift  ← Swift binding + state management
    ↓
ContentView.swift     ← SwiftUI interface
```

## Building

### Xcode (macOS or iOS)

1. Open `PS2Emulator.xcodeproj`
2. Select target: **PS2EmulatorApp**
3. Build for simulator or device: **⌘B**
4. Run: **⌘R**

### Command Line

```bash
cd ps2-emulator-ios
xcodebuild -scheme PS2EmulatorApp -configuration Debug
```

## Usage

### iOS App

1. **Console Tab** — View execution trace and exceptions
2. **Registers Tab** — Inspect R0-R31 and COP0 state
3. **VU0 Tab** — View 32 vector registers (x, y, z, w)
4. **Memory Tab** — Read/write 32-bit words at any address

### Controls

- **Reset** — Clear memory, registers, and PC
- **Step** — Execute one instruction
- **10x** — Execute 10 instructions

### Demo Program

The app loads a default VU0 demo:

```assembly
0x00  VADD  V3 = V1 + V2
0x04  VMUL  V4 = V1 * V2
0x08  VDOT  V5 = dot(V1, V2)
0x0C  VMADD V6 = V1 * V2 + V6
```

Where:
- V1 = {1, 2, 3, 4}
- V2 = {10, 20, 30, 40}

## Phases

- **Phase 9** — Core MIPS CPU
- **Phase 10** — COP0 exception handling
- **Phase 11** — COP2 / VU0 vector unit (current)
- **Phase 12** (planned) — Memory bus, BIOS ROM, I/O registers

## Project Structure

```
ps2-emulator-ios/
├── PS2Emulator/
│   ├── PS2Emulator.h          ← C interface
│   └── PS2Emulator.cpp        ← Core implementation
├── PS2EmulatorApp/
│   ├── ContentView.swift      ← Main UI
│   ├── EmulatorViewModel.swift ← State/binding
│   └── PS2EmulatorApp.swift   ← App entry point
├── Bridging-Header.h          ← C-Swift bridge
├── PS2Emulator.xcodeproj      ← Xcode project
└── README.md
```

## Technical Notes

### Memory Layout

```
0x00000000 - 0x02000000  (32 MB RAM)
├── 0x00000000-0x000000FF   User program space
├── 0x00000180              Exception vector
└── 0x01FFFFFF              RAM boundary
```

### Instruction Encoding

Standard MIPS format:

```
R-Type:  [opcode:6] [rs:5] [rt:5] [rd:5] [shamt:5] [funct:6]
I-Type:  [opcode:6] [rs:5] [rt:5] [imm:16]
J-Type:  [opcode:6] [target:26]
```

### Exception Codes

| Code | Exception | Trigger |
|------|-----------|----------|
| 4    | ADEL      | Load alignment or out-of-bounds |
| 5    | ADES      | Store alignment or out-of-bounds |
| 8    | SYSCALL   | SYSCALL instruction |
| 9    | BREAK     | BREAK instruction |
| 10   | RI        | Reserved/invalid instruction |

## Future Work

- [ ] ELF binary loader
- [ ] DMA controller
- [ ] Graphics interface (GPU commands)
- [ ] Sound processor (SPU)
- [ ] Interrupt controller
- [ ] Debugger (breakpoints, watchpoints)

## License

Educational/prototype use. Not affiliated with Sony Interactive Entertainment.
