#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>

#include "at28c256.h"
#include "bus.h"
#include "decoder.h"
#include "m65c02.h"
#include "mm_clock.h"
#include "op_codes.h"

// Demo program to load into ROM
// A simple program that increments the accumulator
// Load demo program into ROM
void load_demo_program(AT28C256& rom) {
    // Start address 0x8000 (32KB ROM starts at 0x8000 in our memory map)
    // Simple program:
    // 0x8000: LDA #$01    ; Load 1 into accumulator
    // 0x8002: STA $0200   ; Store accumulator to address $0200 (RAM)
    // 0x8005: LDA #$02    ; Load 2 into accumulator
    // 0x8007: STA $0201   ; Store accumulator to address $0201 (RAM)
    // 0x800A: JMP $8000   ; Jump back to start

    // ROM is mapped from 0x8000 to 0xFFFF in the CPU address space
    // But the AT28C256 has 32K of space (0x0000 to 0x7FFF)
    // So we need to start at offset 0 in the EEPROM, which maps to 0x8000 in CPU address space
    word base_addr = 0;  // Base address in EEPROM (will be mapped to 0x8000)

    // LDA #$01 (Load immediate value 1 into accumulator)
    rom.write_byte(base_addr++, static_cast<byte>(Op::LDA_IM));
    rom.write_byte(base_addr++, 0x01);

    // STA $0200 (Store accumulator to address $0200)
    rom.write_byte(base_addr++, static_cast<byte>(Op::STA_ABS));
    rom.write_byte(base_addr++, 0x00);  // Low byte of address
    rom.write_byte(base_addr++, 0x02);  // High byte of address

    // LDA #$02 (Load immediate value 2 into accumulator)
    rom.write_byte(base_addr++, static_cast<byte>(Op::LDA_IM));
    rom.write_byte(base_addr++, 0x02);

    // STA $0201 (Store accumulator to address $0201)
    rom.write_byte(base_addr++, static_cast<byte>(Op::STA_ABS));
    rom.write_byte(base_addr++, 0x01);  // Low byte of address
    rom.write_byte(base_addr++, 0x02);  // High byte of address

    // JMP $8000 (Jump back to start)
    rom.write_byte(base_addr++, static_cast<byte>(Op::JMP));
    rom.write_byte(base_addr++, 0x00);  // Low byte of address
    rom.write_byte(base_addr++, 0x80);  // High byte of address

    // Set reset vector at 0xFFFC-0xFFFD to point to our program start (0x8000)
    // Note: The 6502 uses little-endian format for addresses
    // EEPROM addresses 0x7FFC-0x7FFD correspond to CPU addresses 0xFFFC-0xFFFD
    rom.write_byte(0x7FFC, 0x00);  // Low byte of 0x8000
    rom.write_byte(0x7FFD, 0x80);  // High byte of 0x8000
}

// RAM class implementing the MEM_Module interface
class RAM : public MEM_Module {
   private:
    byte* memory;
    size_t size;

   public:
    RAM(size_t size) : size(size) {
        memory = new byte[size];
        // Initialize memory to 0
        for (size_t i = 0; i < size; ++i) {
            memory[i] = 0;
        }
    }

    ~RAM() { delete[] memory; }

    word read_word(word addr) override {
        if (addr + 1 >= size) {
            std::cerr << "RAM read_word out of bounds: " << std::hex << addr << std::endl;
            return 0xFFFF;
        }
        return static_cast<word>(memory[addr]) | (static_cast<word>(memory[addr + 1]) << 8);
    }

    byte read_byte(byte addr) override {
        if (addr >= size) {
            std::cerr << "RAM read_byte out of bounds: " << std::hex << static_cast<int>(addr) << std::endl;
            return 0xFF;
        }
        return memory[addr];
    }

    void write_word(word addr, word data) override {
        if (addr + 1 >= size) {
            std::cerr << "RAM write_word out of bounds: " << std::hex << addr << std::endl;
            return;
        }
        memory[addr] = data & 0xFF;
        memory[addr + 1] = (data >> 8) & 0xFF;
    }

    void write_byte(byte addr, byte data) override {
        if (addr >= size) {
            std::cerr << "RAM write_byte out of bounds: " << std::hex << static_cast<int>(addr) << std::endl;
            return;
        }
        memory[addr] = data;
    }

    // Debug method to dump RAM contents
    void dump(word start, word length) {
        std::cout << "RAM dump from " << std::hex << start << " to " << (start + length - 1) << ":" << std::endl;
        for (word i = 0; i < length; i += 16) {
            std::cout << std::hex << (start + i) << ": ";
            for (word j = 0; j < 16 && (i + j) < length; ++j) {
                std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(memory[start + i + j])
                          << " ";
            }
            std::cout << std::endl;
        }
    }
};

// EEPROM adapter to implement MEM_Module interface
class EEPROM_Adapter : public MEM_Module {
   private:
    AT28C256& eeprom;

   public:
    EEPROM_Adapter(AT28C256& eeprom) : eeprom(eeprom) {}

    word read_word(word addr) override { return eeprom.read_word(addr); }

    byte read_byte(byte addr) override { return eeprom.read_byte(addr); }

    void write_word(word addr, word data) override { eeprom.write_word(addr, data); }

    void write_byte(byte addr, byte data) override { eeprom.write_byte(addr, data); }
};

int main() {
    std::cout << "Initializing 65C02 computer system..." << std::endl;

    // Create the system bus (40 pins for full system connectivity)
    Bus system_bus(40);

    // Create system components
    M65C02 cpu(system_bus);                       // CPU
    MM_ClockModule clock(1.0);                    // 1 MHz clock
    AT28C256 eeprom(system_bus);                  // 32KB EEPROM
    auto ram = std::make_shared<RAM>(16 * 1024);  // 16KB RAM

    // Create memory mapping adapters
    auto eeprom_adapter = std::make_shared<EEPROM_Adapter>(eeprom);

    // Create address decoder for memory mapping
    AddressDecoder decoder;

    // Set up memory map:
    // - RAM:  0x0000-0x3FFF (16KB)
    // - ROM:  0x8000-0xFFFF (32KB)
    decoder.add_mapping(0x0000, 0x3FFF, ram.get());
    decoder.add_mapping(0x8000, 0xFFFF, eeprom_adapter.get());

    // Load demo program into ROM
    load_demo_program(eeprom);

    // Connect decoder to bus for memory operations
    // Wire up components to access the bus properly

    // Set initial CPU state
    // The CPU reset function will read the reset vector from memory
    cpu.reset();  // This will set PC to the address at reset vector (0xFFFC)

    // Set CPU's program counter to our program start explicitly
    // since we're not fully implementing the memory access in our simulation
    cpu.PC = 0x8000;  // Force PC to start of our program

    // Set clock to manual step mode initially
    clock.smsm();

    bool running = true;
    bool halted = false;
    int iterations = 0;
    const int MAX_ITERATIONS = 3;  // Run 3 complete cycles of our program

    std::cout << "System initialized. Starting execution loop." << std::endl;
    std::cout << "Will execute " << MAX_ITERATIONS << " iterations." << std::endl;

    // Main execution loop
    while (running) {
        // Check if CPU is powered
        if (!system_bus.is_powered()) {
            std::cout << "System power off detected. Exiting..." << std::endl;
            break;
        }

        // Check if HALT line is active (low)
        if (cpu.RDY == 0) {
            if (!halted) {
                std::cout << "CPU halted." << std::endl;
                halted = true;
            }

            // Wait briefly before checking again
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // If we were halted but now RDY is high, resume
        if (halted) {
            std::cout << "CPU resumed." << std::endl;
            halted = false;
        }

        // Check if reset is active (low)
        if (cpu.RESB == 0) {
            std::cout << "CPU reset detected." << std::endl;
            cpu.reset();
            // Debounce reset
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // Execute one clock cycle
        clock.step();

        // On rising edge of clock
        if (clock.CLK == 1) {
            // CPU executes instruction cycle
            // Put CPU address on the address bus
            system_bus.write_address(cpu.PC);

            // Read current instruction from the address decoder
            byte opcode = decoder.read(cpu.PC);

            // Signal on the bus that we're reading
            system_bus.bytes.BYTE_4 = 1;  // Read mode

            // Simple execution model (in a real implementation, this would be more complex)
            // Increment PC
            cpu.PC++;

            // Dummy handler for some opcodes to demonstrate
            switch (opcode) {
                case static_cast<byte>(Op::LDA_IM): {
                    // Load immediate value into A
                    byte value = decoder.read(cpu.PC++);
                    cpu.A = value;
                    std::cout << "Executed LDA #$" << std::hex << static_cast<int>(value)
                              << " - A=" << static_cast<int>(cpu.A) << std::endl;
                    break;
                }
                case static_cast<byte>(Op::STA_ABS): {
                    // Store A to absolute address
                    byte low = decoder.read(cpu.PC++);
                    byte high = decoder.read(cpu.PC++);
                    word addr = (static_cast<word>(high) << 8) | low;
                    decoder.write(addr, cpu.A);
                    std::cout << "Executed STA $" << std::hex << addr << " - Stored " << static_cast<int>(cpu.A)
                              << std::endl;
                    break;
                }
                case static_cast<byte>(Op::JMP): {
                    // Jump to absolute address
                    byte low = decoder.read(cpu.PC++);
                    byte high = decoder.read(cpu.PC++);
                    word addr = (static_cast<word>(high) << 8) | low;
                    std::cout << "Executed JMP $" << std::hex << addr << " - PC was " << cpu.PC << std::endl;

                    // Increment iteration counter when we jump back to start
                    if (addr == 0x8000) {
                        iterations++;
                        std::cout << "\n=== Completed iteration " << iterations << " of " << MAX_ITERATIONS
                                  << " ===" << std::endl;

                        // Check if we've reached our limit
                        if (iterations >= MAX_ITERATIONS) {
                            std::cout << "Reached maximum iterations. Stopping execution." << std::endl;
                            running = false;
                            break;
                        }
                    }

                    cpu.PC = addr;
                    break;
                }
                case static_cast<byte>(Op::NOP): {
                    std::cout << "Executed NOP" << std::endl;
                    break;
                }
                case static_cast<byte>(Op::BRK): {
                    std::cout << "BRK instruction detected. Stopping execution." << std::endl;
                    running = false;
                    break;
                }
                default: {
                    std::cout << "Unknown opcode: 0x" << std::hex << static_cast<int>(opcode) << " at PC=0x"
                              << std::setw(4) << std::setfill('0') << (cpu.PC - 1) << std::endl;
                    // In a real system, we might want to trigger an interrupt or halt
                    std::cout << "Unimplemented instruction. Stopping execution." << std::endl;
                    running = false;
                    break;
                }
            }

            // Slow down execution for demonstration purposes
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            // Periodically dump RAM for debugging
            if (cpu.PC % 10 == 0) {
                std::cout << "\nMemory status at 0x0200:" << std::endl;
                ram->dump(0x0200, 16);  // Show first 16 bytes of our RAM data area
            }
        }
    }

    std::cout << "\n---------------------------------------" << std::endl;
    std::cout << "Execution complete. Final CPU state:" << std::endl;
    std::cout << "A: 0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(cpu.A) << " X: 0x"
              << std::setw(2) << std::setfill('0') << static_cast<int>(cpu.X) << " Y: 0x" << std::setw(2)
              << std::setfill('0') << static_cast<int>(cpu.Y) << " PC: 0x" << std::setw(4) << std::setfill('0')
              << static_cast<int>(cpu.PC) << " SP: 0x" << std::setw(2) << std::setfill('0') << static_cast<int>(cpu.SP)
              << std::endl;
    std::cout << "Flags: " << (cpu.FLAGS_N ? "N" : "-") << (cpu.FLAGS_V ? "V" : "-") << (cpu.FLAGS_U ? "U" : "-")
              << (cpu.FLAGS_B ? "B" : "-") << (cpu.FLAGS_D ? "D" : "-") << (cpu.FLAGS_I ? "I" : "-")
              << (cpu.FLAGS_Z ? "Z" : "-") << (cpu.FLAGS_C ? "C" : "-") << std::endl;

    std::cout << "\nFinal RAM state:" << std::endl;
    ram->dump(0x0200, 16);

    return 0;
}
