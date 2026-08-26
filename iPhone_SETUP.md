# PS2 Emulator Standalone C++ (iPhone)

## รันบน iPhone

### Option 1: Xcode บน Mac (recommended)

```bash
cd ps2-emulator-ios
open ps2_emulator_standalone.cpp
```

1. สร้าง **New Xcode Project**
   - Select: **macOS** > **Command Line Tool**
   - Language: **C++**

2. Copy `ps2_emulator_standalone.cpp` ลงใน project

3. Build: **⌘B**

4. Run: **⌘R**

### Option 2: Compile บน Mac และ Copy ลง iPhone

```bash
# Compile สำหรับ ARM64 (iPhone)
clang++ -std=c++17 -arch arm64 -o ps2_emulator ps2_emulator_standalone.cpp

# Copy ลง iPhone ผ่าน Xcode
```

### Option 3: Deploy เป็น iOS App

ใช้ SwiftUI wrapper (จากโปรเจค iOS App อื่น)

---

## Output ที่ได้

```
================================
  PS2 Emulator Phase 11
  iPhone C++ Standalone
================================

[INIT] V1 = {1, 2, 3, 4}
[INIT] V2 = {10, 20, 30, 40}

[DEMO] Program loaded at 0x00

Cycle 1
PC=0x00000000 Instr=0x4c420842
  VADD V3 = V1 + V2

Cycle 2
PC=0x00000004 Instr=0x4c420904
  VMUL V4 = V1 * V2

Cycle 3
PC=0x00000008 Instr=0x4c4209c2
  VDOT V5 = dot(V1, V2)

Cycle 4
PC=0x0000000c Instr=0x4c420a00
  VMADD V6 = V1 * V2 + V6

================================
  Results
================================
V3 = {11, 22, 33, 44}
V4 = {10, 40, 90, 160}
V5 = {300, 300, 300, 300}
V6 = {10, 40, 90, 160}

================================
  Complete!
================================
```
