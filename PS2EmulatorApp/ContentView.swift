import SwiftUI

struct ContentView: View {
    @StateObject private var viewModel = EmulatorViewModel()
    @State private var selectedTab = 0
    
    var body: some View {
        ZStack {
            Color(.systemBackground).ignoresSafeArea()
            
            VStack(spacing: 0) {
                HStack {
                    Text("PS2 Emulator Phase 11")
                        .font(.system(size: 20, weight: .bold, design: .monospaced))
                    Spacer()
                    Text("PC: 0x\(String(format: "%08X", viewModel.pc))")
                        .font(.system(size: 12, weight: .regular, design: .monospaced))
                        .foregroundColor(.green)
                }
                .padding(.horizontal, 12)
                .padding(.vertical, 10)
                .background(Color(.systemGray6))
                
                Picker("View", selection: $selectedTab) {
                    Text("Console").tag(0)
                    Text("Registers").tag(1)
                    Text("VU0").tag(2)
                    Text("Memory").tag(3)
                }
                .pickerStyle(.segmented)
                .padding(12)
                
                if selectedTab == 0 {
                    ConsoleView(output: viewModel.output)
                } else if selectedTab == 1 {
                    RegistersView(registers: viewModel.registers, cop0: viewModel.cop0)
                } else if selectedTab == 2 {
                    VU0View(vuRegisters: viewModel.vuRegisters)
                } else {
                    MemoryView(viewModel: viewModel)
                }
                
                HStack(spacing: 8) {
                    Button(action: viewModel.reset) {
                        HStack {
                            Image(systemName: "arrow.counterclockwise")
                            Text("Reset")
                        }
                        .frame(maxWidth: .infinity)
                    }
                    .buttonStyle(.bordered)
                    
                    Button(action: viewModel.step) {
                        HStack {
                            Image(systemName: "play.fill")
                            Text("Step")
                        }
                        .frame(maxWidth: .infinity)
                    }
                    .buttonStyle(.borderedProminent)
                    
                    Button(action: { viewModel.runSteps(10) }) {
                        HStack {
                            Image(systemName: "forward.fill")
                            Text("10x")
                        }
                        .frame(maxWidth: .infinity)
                    }
                    .buttonStyle(.bordered)
                }
                .padding(12)
            }
        }
    }
}

struct ConsoleView: View {
    let output: String
    @State private var scrollPosition: CGFloat = 0
    
    var body: some View {
        VStack {
            ScrollView {
                VStack(alignment: .leading, spacing: 2) {
                    Text(output.isEmpty ? "[Ready]" : output)
                        .font(.system(size: 11, weight: .regular, design: .monospaced))
                        .foregroundColor(.green)
                        .textSelection(.enabled)
                        .frame(maxWidth: .infinity, alignment: .leading)
                }
                .padding(8)
            }
            .background(Color.black)
            .cornerRadius(6)
            .padding(12)
        }
    }
}

struct RegistersView: View {
    let registers: [UInt32]
    let cop0: [UInt32]
    
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 0) {
                Text("General Purpose Registers")
                    .font(.system(size: 14, weight: .bold, design: .monospaced))
                    .padding(.horizontal, 12)
                    .padding(.vertical, 8)
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .background(Color(.systemGray5))
                
                LazyVGrid(columns: [GridItem(.flexible()), GridItem(.flexible())], spacing: 1) {
                    ForEach(0..<32, id: \.self) { i in
                        VStack(alignment: .leading, spacing: 2) {
                            Text("R\(i)")
                                .font(.system(size: 12, weight: .bold, design: .monospaced))
                            Text("0x\(String(format: "%08X", registers[i]))")
                                .font(.system(size: 10, weight: .regular, design: .monospaced))
                                .foregroundColor(.blue)
                        }
                        .padding(6)
                        .background(Color(.systemGray6))
                        .cornerRadius(4)
                    }
                }
                .padding(12)
                
                Text("COP0 Registers")
                    .font(.system(size: 14, weight: .bold, design: .monospaced))
                    .padding(.horizontal, 12)
                    .padding(.vertical, 8)
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .background(Color(.systemGray5))
                
                VStack(alignment: .leading, spacing: 4) {
                    HStack {
                        Text("Status (12)").frame(maxWidth: .infinity, alignment: .leading)
                        Text("0x\(String(format: "%08X", cop0[12]))")
                    }
                    .font(.system(size: 11, design: .monospaced))
                    HStack {
                        Text("Cause (13)").frame(maxWidth: .infinity, alignment: .leading)
                        Text("0x\(String(format: "%08X", cop0[13]))")
                    }
                    .font(.system(size: 11, design: .monospaced))
                    HStack {
                        Text("EPC (14)").frame(maxWidth: .infinity, alignment: .leading)
                        Text("0x\(String(format: "%08X", cop0[14]))")
                    }
                    .font(.system(size: 11, design: .monospaced))
                }
                .padding(12)
            }
        }
    }
}

struct VU0View: View {
    let vuRegisters: [(Float, Float, Float, Float)]
    
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 0) {
                Text("VU0 Vector Registers (128-bit)")
                    .font(.system(size: 14, weight: .bold, design: .monospaced))
                    .padding(.horizontal, 12)
                    .padding(.vertical, 8)
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .background(Color(.systemGray5))
                
                VStack(alignment: .leading, spacing: 4) {
                    ForEach(0..<32, id: \.self) { i in
                        let (x, y, z, w) = vuRegisters[i]
                        VStack(alignment: .leading, spacing: 2) {
                            Text("V\(i)")
                                .font(.system(size: 11, weight: .bold, design: .monospaced))
                            Text("{ \(String(format: "%.3f", x)), \(String(format: "%.3f", y)), \(String(format: "%.3f", z)), \(String(format: "%.3f", w)) }")
                                .font(.system(size: 9, weight: .regular, design: .monospaced))
                                .foregroundColor(.purple)
                        }
                        .padding(6)
                        .background(Color(.systemGray6))
                        .cornerRadius(4)
                    }
                }
                .padding(12)
            }
        }
    }
}

struct MemoryView: View {
    @ObservedObject var viewModel: EmulatorViewModel
    @State private var addressInput = "0x00000000"
    
    var body: some View {
        VStack(spacing: 12) {
            HStack {
                TextField("Address (hex)", text: $addressInput)
                    .textFieldStyle(.roundedBorder)
                    .font(.system(size: 12, design: .monospaced))
                    .autocorrectionDisabled()
                
                Button("Read") {
                    if let addr = UInt32(addressInput.replacingOccurrences(of: "0x", with: ""), radix: 16) {
                        viewModel.readMemory(addr)
                    }
                }
                .buttonStyle(.bordered)
            }
            .padding(12)
            
            if let value = viewModel.memoryValue {
                VStack(alignment: .leading, spacing: 4) {
                    Text("Memory Value")
                        .font(.system(size: 12, weight: .bold, design: .monospaced))
                    Text("0x\(String(format: "%08X", value))")
                        .font(.system(size: 16, weight: .bold, design: .monospaced))
                        .foregroundColor(.cyan)
                }
                .padding(12)
                .background(Color(.systemGray6))
                .cornerRadius(6)
                .padding(.horizontal, 12)
            }
            
            Spacer()
        }
    }
}

#Preview {
    ContentView()
}
