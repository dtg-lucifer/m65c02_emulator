#include <algorithm>  // For std::max
#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>

#include "at28c256.h"
#include "bus.h"
#include "decoder.h"
#include "hm62256b.h"
#include "log.h"
#include "mm_clock.h"
#include "wdc65c02.h"

// Enhanced test program with multiple instructions
const byte EXAMPLE_PROGRAM[] = {
    0xA9, 0x42,        // LDA #$42 - Load the value 0x42 into the A register
    0xA2, 0x08,        // LDX #$08 - Load the value 0x08 into the X register
    0xA0, 0x15,        // LDY #$15 - Load the value 0x15 into the Y register
    0x8D, 0x00, 0x20,  // STA $2000 - Store the value in A to memory location $2000
    0xE8,              // INX - Increment X register
    0x88,              // DEY - Decrement Y register
    0xAA,              // TAX - Transfer A to X
    0x98,              // TYA - Transfer Y to A
    0xEA,              // NOP - No operation
    0x00               // BRK - Break (halt CPU)
};

void load_program(AT28C256& eeprom, const byte* program, size_t size, word start_addr = 0x8000) {
    // Calculate the local offset for the EEPROM (removing the 0x8000 base)
    word local_addr = start_addr - 0x8000;

    // Load the program into EEPROM
    logger::info("Loading program into EEPROM at 0x" + std::to_string(start_addr) + ":");
    for (size_t i = 0; i < size; i++) {
        eeprom.memory[local_addr + i] = program[i];
        // Only log every few bytes to reduce output noise
        if (i % 4 == 0 || i == size - 1) {
            std::stringstream ss;
            ss << "  0x" << std::hex << std::setfill('0') << std::setw(4) << (start_addr + i) << ": 0x" << std::hex
               << std::setfill('0') << std::setw(2) << (int)program[i];
            logger::info(ss.str());
        }
    }

    // Write reset vector at 0xFFFC to point to our program
    byte low_byte = start_addr & 0xFF;
    byte high_byte = (start_addr >> 8) & 0xFF;

    // Write reset vector to EEPROM (at offset 0x7FFC from the base of 0x8000)
    word reset_vector_offset = 0x7FFC;                   // 0xFFFC - 0x8000
    eeprom.memory[reset_vector_offset] = low_byte;       // Low byte
    eeprom.memory[reset_vector_offset + 1] = high_byte;  // High byte

    std::stringstream ss;
    ss << "Reset vector set to 0x" << std::hex << std::setfill('0') << std::setw(4) << ((high_byte << 8) | low_byte)
       << " (0x" << std::hex << std::setfill('0') << std::setw(2) << (int)high_byte << std::setfill('0') << std::setw(2)
       << (int)low_byte << ")";
    logger::info(ss.str());

    // Verify reset vector was written correctly
    byte read_low = eeprom.memory[reset_vector_offset];
    byte read_high = eeprom.memory[reset_vector_offset + 1];
    std::stringstream ss2;
    ss2 << "Verified reset vector: 0x" << std::hex << std::setfill('0') << std::setw(4)
        << ((read_high << 8) | read_low);
    logger::info(ss2.str());
}

int main() {
    logger::print("WDC65C02 Computer Simulator");
    logger::info("Initializing components...");

    try {
        // Create the shared bus
        Bus system_bus(40);  // 40-bit wide bus to accommodate all signals

        // ===================================================
        // CLOCK SPEED CONFIGURATION - ADJUST THIS VALUE BELOW
        // ===================================================
        // Lower values = slower execution, higher values = faster execution
        // Examples:
        // - 0.5 Hz = Very slow, approximately 2 seconds per instruction
        // - 2.0 Hz = Moderate speed, approximately 0.5 seconds per instruction
        // - 10.0 Hz = Fast, approximately 0.1 seconds per instruction
        // - 50.0 Hz = Very fast execution, useful for quick testing
        MM_ClockModule clock(2.0f, ClockMode::A_STABLE);

        // Create memory modules
        AT28C256 eeprom(system_bus);  // EEPROM for ROM (0x8000-0xFFFF)
        HM62256B sram(system_bus);    // SRAM for RAM (0x0000-0x7FFF)

        // Create address decoder and configure memory map
        AddressDecoder decoder;
        decoder.add_mapping(0x0000, 0x7FFF, &sram);    // SRAM at 0x0000-0x7FFF
        decoder.add_mapping(0x8000, 0xFFFF, &eeprom);  // EEPROM at 0x8000-0xFFFF

        // Create CPU and attach to bus with decoder
        WDC65C02 cpu(system_bus);
        cpu.set_decoder(&decoder);  // Explicitly set the decoder

        // Load example program into EEPROM - now correctly at 0x8000
        logger::header("LOADING PROGRAM DATA");
        load_program(eeprom, EXAMPLE_PROGRAM, sizeof(EXAMPLE_PROGRAM), 0x8000);
        logger::info("Program loaded successfully. Size: " + std::to_string(sizeof(EXAMPLE_PROGRAM)) + " bytes");
        logger::divider();

        // Start the clock module
        logger::info("Starting clock module in continuous mode");
        clock.sasm();   // Use A_STABLE mode for continuous clocking
        clock.CLK = 1;  // Set initial clock state to HIGH
        logger::info("Clock speed set to: " + std::to_string(clock.get_speed()) + " Hz");
        clock.start();

        // Connect clock to CPU - share the clock object
        // We need to add a polling loop to keep the clock connected to the CPU
        std::thread clock_connect_thread([&cpu, &clock]() {
            // Run silently to reduce log noise
            while (true) {
                // Update CPU clock pin from clock module
                cpu.PHI0 = clock.CLK;

                // Small sleep to prevent high CPU usage
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });

        // Detach the thread so it runs independently
        clock_connect_thread.detach();

        // Start the memory monitoring
        logger::info("Starting memory modules monitoring...");
        eeprom.start_monitoring();
        sram.start_monitoring();

        // Configure EEPROM pins
        eeprom.CE = 0;  // Chip Enable active (low)
        eeprom.OE = 0;  // Output Enable active (low) for reading
        eeprom.WE = 1;  // Write Enable inactive (high) for reading

        // Configure SRAM pins
        sram.CS = 0;  // Chip Select active (low)
        sram.OE = 0;  // Output Enable active (low) for reading
        sram.WE = 1;  // Write Enable inactive (high) for reading

        // We need to connect the CPU's address pins to the bus and the EEPROM
        // This simulates the physical connections in a real computer
        system_bus.PINS = 0;  // Clear all pins

        // Manually read the reset vector through the decoder
        // This simulates what happens during the CPU's reset sequence
        word reset_vector_addr = 0xFFFC;
        byte low_byte = 0x00;
        byte high_byte = 0x80;  // Hardcode to 0x8000 for predictability

        // Set the reset vector in EEPROM
        word eeprom_offset = reset_vector_addr - 0x8000;  // Calculate offset in EEPROM
        if (eeprom_offset < 32 * 1024) {
            eeprom.memory[eeprom_offset] = low_byte;
            eeprom.memory[eeprom_offset + 1] = high_byte;
        }

        // Now read it through the decoder to verify
        low_byte = decoder.read(reset_vector_addr);
        high_byte = decoder.read(reset_vector_addr + 1);
        word program_start = (high_byte << 8) | low_byte;

        std::stringstream ss;
        ss << "Reset vector read from 0x" << std::hex << std::setfill('0') << std::setw(4) << reset_vector_addr
           << ": 0x" << std::hex << std::setfill('0') << std::setw(2) << (int)low_byte << std::hex << std::setfill('0')
           << std::setw(2) << (int)high_byte << " (points to 0x" << std::hex << std::setfill('0') << std::setw(4)
           << program_start << ")";
        logger::info(ss.str());

        // Boot and start the CPU
        logger::header("BOOTING CPU");
        cpu.boot();

        // Manually set PC to the program start (bypassing reset vector read issues)
        cpu.PC = program_start;
        {
            std::stringstream ss;
            ss << "CPU booted. Initial PC=0x" << std::hex << std::setfill('0') << std::setw(4) << cpu.PC;
            logger::info(ss.str());
        }

        // Copy the first instruction to the bus for the CPU to fetch
        system_bus.write_address(cpu.PC);
        byte first_instr = eeprom.memory[cpu.PC];
        system_bus.write_data(first_instr);
        std::stringstream first_instr_ss;
        first_instr_ss << "Initial instruction at PC=0x0000 is 0x" << std::hex << std::setfill('0') << std::setw(2)
                       << (int)first_instr;
        logger::info(first_instr_ss.str());

        logger::header("CONNECTING MEMORY SYSTEM");
        // Create a connection between memory and bus/CPU through the decoder
        // This thread ensures that memory access happens correctly for all components
        std::thread memory_connect_thread([&system_bus, &decoder, &eeprom, &sram]() {
            // Run silently in background
            while (true) {
                // Read address from bus
                word addr = system_bus.read_address();

                // Use the appropriate memory module based on the address
                if (addr < 0x8000) {
                    // Address is in SRAM range (0x0000-0x7FFF)
                    byte data = sram.memory[addr];
                    system_bus.write_data(data);
                } else {
                    // Address is in EEPROM range (0x8000-0xFFFF)
                    word eeprom_addr = addr - 0x8000;
                    if (eeprom_addr < 32 * 1024) {
                        byte data = eeprom.memory[eeprom_addr];
                        system_bus.write_data(data);
                    }
                }

                // Reduced polling frequency to lower CPU usage
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });

        // Detach the thread so it runs independently
        memory_connect_thread.detach();

        // Start execution
        cpu.execute();

        // Main program loop - keep running until user interrupts or CPU halts
        logger::header("STARTING CPU EXECUTION");
        int total_cycles = 0;
        try {
            word prev_pc = 0xFFFF;  // Initialize to an invalid value to ensure first cycle is always logged
            byte prev_a = 0xFF, prev_x = 0xFF, prev_y = 0xFF;  // Previous register values
            byte prev_flags = 0xFF;                            // Previous flags value

            while (cpu.state != CPU_State::HALTED && total_cycles < 100) {  // Allow up to 100 cycles for our program

                // Determine which memory module to read from based on PC
                byte current_instr;
                if (cpu.PC < 0x8000) {
                    // Read from SRAM
                    current_instr = sram.memory[cpu.PC];
                } else {
                    // Read from EEPROM (with offset)
                    word eeprom_addr = cpu.PC - 0x8000;
                    if (eeprom_addr < 32 * 1024) {
                        current_instr = eeprom.memory[eeprom_addr];
                    } else {
                        current_instr = 0xEA;  // Default to NOP if out of range
                    }
                }

                // Only log if PC changed (meaning we're about to execute a new instruction)
                bool should_log = (cpu.PC != prev_pc);

                // Force the opcode at PC into the bus so CPU can fetch it
                if (system_bus.request_bus(BusOwner::CPU)) {
                    system_bus.write_address(cpu.PC);
                    system_bus.write_data(current_instr);
                    system_bus.release_bus(BusOwner::CPU);
                }

                // Only print cycle information if we're at a new instruction
                if (should_log) {
                    total_cycles++;

                    // Create a nicely formatted cycle header
                    std::stringstream cycle_header;
                    cycle_header << "CPU CYCLE " << std::setfill(' ') << std::setw(3) << std::right << total_cycles;
                    logger::subheader(cycle_header.str());

                    // Create a simplified flags string
                    std::string flags = "";
                    if (cpu.FLAGS_N) flags += "N";
                    if (cpu.FLAGS_Z) flags += "Z";
                    if (cpu.FLAGS_C) flags += "C";
                    if (cpu.FLAGS_I) flags += "I";
                    if (flags.empty()) flags = "-";

                    // Log CPU state before execution
                    {
                        std::stringstream ss;
                        ss << "CPU State: PC=0x" << std::hex << std::setfill('0') << std::setw(4) << cpu.PC << ", A=0x"
                           << std::hex << std::setfill('0') << std::setw(2) << (int)cpu.A << ", X=0x"
                           << std::setfill('0') << std::setw(2) << (int)cpu.X << ", Y=0x" << std::setfill('0')
                           << std::setw(2) << (int)cpu.Y << " | Flags: " << flags;
                        logger::info(ss.str());
                    }

                    // Log the next instruction to be executed
                    {
                        std::stringstream ss;
                        ss << "Next instruction: 0x" << std::hex << std::setfill('0') << std::setw(2)
                           << (int)current_instr << " at PC=0x" << std::hex << std::setfill('0') << std::setw(4)
                           << cpu.PC;
                        logger::info(ss.str());
                    }
                }

                // Save current state to detect changes
                prev_pc = cpu.PC;
                prev_a = cpu.A;
                prev_x = cpu.X;
                prev_y = cpu.Y;
                prev_flags = cpu.FLAGS;

                // Check if CPU has halted
                if (cpu.state == CPU_State::HALTED) {
                    logger::info("CPU halted. Exiting...");
                    break;
                }

                // Set PHI0 high to allow execution
                cpu.PHI0 = 1;

                // Call execute_instruction directly
                cpu.execute_instruction();

                // Only log state after execution if something changed
                if (prev_pc != cpu.PC || prev_a != cpu.A || prev_x != cpu.X || prev_y != cpu.Y ||
                    prev_flags != cpu.FLAGS) {
                    // Create a more complete flags string
                    std::string flags = "";
                    if (cpu.FLAGS_N) flags += "N";
                    if (cpu.FLAGS_Z) flags += "Z";
                    if (cpu.FLAGS_C) flags += "C";
                    if (cpu.FLAGS_I) flags += "I";
                    if (flags.empty()) flags = "-";

                    // Format the output with consistent style
                    std::stringstream cpu_state;
                    cpu_state << "After execution: PC=0x" << std::hex << std::setfill('0') << std::setw(4) << cpu.PC
                              << ", A=0x" << std::hex << std::setfill('0') << std::setw(2) << (int)cpu.A << ", X=0x"
                              << std::setfill('0') << std::setw(2) << (int)cpu.X << ", Y=0x" << std::setfill('0')
                              << std::setw(2) << (int)cpu.Y << " | Flags: " << flags;
                    logger::info(cpu_state.str());

                    // Add a separator line for better readability
                    logger::divider();
                }

                // Set PHI0 low for next cycle
                cpu.PHI0 = 0;

                // Delay between instructions based on clock speed
                // The formula below gives half cycle time (in milliseconds)
                // - At 1Hz: 500ms delay (2 instructions per second)
                // - At 0.5Hz: 1000ms delay (1 instruction per second)
                // - At 10Hz: 50ms delay (20 instructions per second)
                int delay_ms = static_cast<int>(500.0 / clock.get_speed());
                delay_ms = std::max(1, delay_ms);  // Ensure minimum delay of 1ms
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            }
        } catch (const std::exception& e) {
            logger::error("Exception: " + std::string(e.what()));
        }

        if (cpu.state == CPU_State::HALTED) {
            logger::header("CPU EXECUTION HALTED");

            // Create a more complete flags string
            std::string flags = "";
            if (cpu.FLAGS_N) flags += "N";
            if (cpu.FLAGS_Z) flags += "Z";
            if (cpu.FLAGS_C) flags += "C";
            if (cpu.FLAGS_I) flags += "I";
            if (flags.empty()) flags = "-";

            // Format the final CPU state nicely
            std::stringstream ss;
            ss << "Final Register Values:";
            logger::info(ss.str());

            std::stringstream ss2;
            ss2 << "  A = 0x" << std::hex << std::setfill('0') << std::setw(2) << (int)cpu.A << "  X = 0x"
                << std::setfill('0') << std::setw(2) << (int)cpu.X << "  Y = 0x" << std::setfill('0') << std::setw(2)
                << (int)cpu.Y;
            logger::info(ss2.str());

            std::stringstream ss3;
            ss3 << "  PC = 0x" << std::hex << std::setfill('0') << std::setw(4) << cpu.PC << "  Flags = " << flags;
            logger::info(ss3.str());

            std::stringstream ss4;
            ss4 << "Total cycles executed: " << std::dec << total_cycles;
            logger::info(ss4.str());
        } else {
            logger::header("EXECUTION LIMIT REACHED");
            logger::info("Program did not finish. Total cycles: " + std::to_string(total_cycles));
        }
        logger::header("EXECUTION COMPLETE");
        logger::info("Shutting down system...");
        return 0;
    } catch (const std::exception& e) {
        logger::error("Exception in main: " + std::string(e.what()));
    } catch (...) {
        logger::error("Unknown exception in main");
    }
    return 1;
}
