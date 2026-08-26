# Setup Instructions

## Prerequisites

- macOS 12.0+ or iOS 15.0+
- Xcode 14.0+
- Swift 5.7+

## Quick Start (macOS/Simulator)

### 1. Clone the Repository

```bash
git clone https://github.com/ping1911-byte/ps2-emulator-ios.git
cd ps2-emulator-ios
```

### 2. Open in Xcode

```bash
open PS2Emulator.xcodeproj
```

### 3. Build

```bash
xcodebuild -scheme PS2EmulatorApp -configuration Debug
```

### 4. Run on Simulator

```bash
xcodebuild -scheme PS2EmulatorApp -configuration Debug -destination 'platform=iOS Simulator,name=iPhone 15'
```

Or in Xcode: **⌘R**

## Xcode Project Configuration

The project includes:

- **Bridging Header** — Links C++ (PS2Emulator.cpp) to Swift
- **Build Settings** — C++ language dialect (C++17)
- **Linking** — No external dependencies

### File Roles

| File | Role | Language |
|------|------|----------|
| `PS2Emulator.h` | Public C interface | C |
| `PS2Emulator.cpp` | Core emulator | C++ |
| `Bridging-Header.h` | Swift-C bridge | C |
| `EmulatorViewModel.swift` | State management | Swift |
| `ContentView.swift` | UI | SwiftUI |
| `PS2EmulatorApp.swift` | App entry | Swift |

## Troubleshooting

### "Bridging header not found"

In Xcode: **Build Settings** → search "bridging header"
→ Set **Bridging Header** to `Bridging-Header.h`

### "PS2Emulator.h not found"

In Xcode: **Build Settings** → search "header search paths"
→ Add `$(SRCROOT)/PS2Emulator`

### Compilation errors in `.cpp`

Ensure the target's **Compile Sources** includes:
- `PS2Emulator.cpp`
- NOT `PS2Emulator.h` (header only)

## Running Demo Program

When the app launches:

1. Default VU0 demo is loaded (initializes V1, V2)
2. Tap **Console** to see output
3. Tap **Step** to execute one instruction
4. Tap **10x** to run 10 instructions
5. Switch to **VU0** tab to see vector results

## Next Steps

- Extend the demo program in `EmulatorViewModel.loadDemoProgram()`
- Add more MIPS instructions to `PS2Emulator.cpp` → `ps2_execute()`
- Implement Phase 12 features (ROM, I/O)

## Building for Device

1. Xcode: **Product** → **Destination** → Select device
2. **⌘B** to build
3. Resolve signing: **Project** → **Signing & Capabilities** → Team

## Command-Line Build (CI/CD)

```bash
xcodebuild \
  -scheme PS2EmulatorApp \
  -configuration Release \
  -destination generic/platform=iOS \
  -derivedDataPath build
```

## Debugging

### Xcode Debugger

1. **Debug** → **Breakpoints** → Set breakpoint in `.cpp` or `.swift`
2. **Debug** → **Activate Breakpoints** (⌘Y)
3. Run and inspect state

### Console Output

The app prints to console via `os_log` and the in-app text view. Use:

```swift
print(viewModel.output) // Swift
NSLog("%s", ps2_get_output(emu)); // Objective-C
```

## Performance Tips

- Simulator: Use Latest iOS SDK for faster emulation
- Device: Test on physical hardware for real performance
- Large memory reads: Implement paging for 32 MB address space

## Distribution

### TestFlight

1. In Xcode: **Product** → **Archive**
2. **Organizer** → **Distribute**
3. Select **TestFlight**

### App Store

1. Ensure EULA/usage rights (educational/simulation)
2. **Product** → **Archive** → **Distribute**
3. Select **App Store Connect**
