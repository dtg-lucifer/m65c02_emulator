#include "wdc65c02.h"

#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

#include "bus.h"
#include "log.h"
#include "op_codes.h"

WDC65C02::WDC65C02(Bus& bus, AddressDecoder* decoder) : bus(bus) {
    // Clear registers
    for (int i = 0; i < 3; ++i) registers[i] = 0;

    PC = 0x0000;            // Will be set by reset() from vector
    SP = 0xFF;              // Stack starts at top of page 1
    FLAGS = 0x34;           // Default reset state (IRQ disabled, U=1)
    decoder_ptr = decoder;  // Initialize decoder pointer

    // Power pins
    VCC = 1, VSS1 = 0, VSS2 = 0;

    // Control pins
    RESB = 1, RWB = 1, SYNC = 0;
    RDY = 1, IRQB = 1, NMIB = 1;

    // Clocks
    PHI0 = 0, PHI1O = 0, PHI2O = 0;

    // Buses
    this->ADDR = 0x0000;
    this->DATA = 0x00;

    state = CPU_State::POWER_OFF;
}

void WDC65C02::reset() {
    CPU_State& old_state = this->state;  // Store the old state
    this->state = CPU_State::RESET;      // Set the CPU state to RESET

    // Set the PC and SP to their initial position
    this->SP = 0xFF;    // Stack pointer starts at 0xFF (top of stack)
    this->PC = 0x0000;  // Program counter starts at beginning of memory - will read reset vector

    for (int i = 0; i < 3; ++i) {
        this->registers[i] = 0;  // Initialize all registers to zero
    }

    this->FLAGS = 0;    // Clear all flags
    this->FLAGS_I = 1;  // Set Interrupt Disable Flag (I) to 1

    // R/W high to read from the memory
    this->PHI0 = 0;  // Set PHI0 to low (inactive state)
    this->SYNC = 1;  // Set SYNC to high (not in sync state)

    // Set the address to the reset vector
    // to fetch the location of the program to be executed
    // for the first time after boot
    this->RWB = 1;  // Set R/W to high for read operation

    if (bus.request_bus(BusOwner::CPU)) {
        logger::info("CPU obtained bus for reset vector read");
        this->bus.write_address(this->PC);  // write the reset vector address to be read
        byte lo = this->bus.read_data();    // read the low byte
        std::stringstream ss;
        ss << "Reset vector low byte: 0x" << std::hex << (int)lo;
        logger::info(ss.str());

        this->PC++;                         // Increment to read high byte
        this->bus.write_address(this->PC);  // write the incremented address
        byte hi = this->bus.read_data();    // read the high byte

        std::stringstream ss2;
        ss2 << "Reset vector high byte: 0x" << std::hex << (int)hi;
        logger::info(ss2.str());

        word start_addr = (hi << 8) | lo;  // Combine high and low byte to form the address

        std::stringstream ss3;
        ss3 << "Setting PC to start address: 0x" << std::hex << start_addr;
        logger::info(ss3.str());
        this->PC = start_addr;  // Set PC to the program start address

        bus.release_bus(BusOwner::CPU);
    } else {
        logger::error("Failed to get bus access during reset");
        // Fallback to a default address if we can't read the reset vector
        this->PC = 0x0000;
    }

    // return to the old state
    this->state = old_state;  // Restore the previous state
}

void WDC65C02::attach_to_bus(Bus& bus) {
    this->bus = bus;  // Attach the bus reference
}

void WDC65C02::set_decoder(AddressDecoder* decoder) {
    this->decoder_ptr = decoder;  // Set pointer to address decoder
    logger::info("CPU access to memory through address decoder established");
}

byte WDC65C02::read_byte() {
    this->RWB = 1;                      // Set R/W to high for read operation
    this->bus.write_address(this->PC);  // Write the address to be read to the bus
    byte data;
    if (decoder_ptr) {
        data = decoder_ptr->read(this->PC);  // Use decoder to read memory
    } else {
        data = this->bus.read_data();  // Fall back to bus if no decoder
    }
    return data;  // Return the read byte
}

word WDC65C02::read_word() {
    word old_pc = this->PC;             // Store the old program counter
    this->RWB = 1;                      // Set R/W to high for read operation
    this->bus.write_address(this->PC);  // Write the address to be read to the bus
    byte lo = this->bus.read_data();    // Read the low byte
    this->PC++;                         // Increment program counter after reading
    byte hi = this->bus.read_data();    // Read the high byte
    this->PC = old_pc;                  // Restore the program counter to the old value
    return (hi << 8) | lo;              // Combine high and low byte to form the word
}

byte WDC65C02::fetch_byte() {
    // We no longer need bounds checking here since the decoder will handle
    // the memory access bounds checking for each module
    // Just log the PC value for debugging purposes
    if (this->PC > 0xFFFF) {  // This should never happen with 16-bit address
        std::stringstream ss;
        ss << "Invalid program counter value: PC=0x" << std::hex << std::setfill('0') << std::setw(4) << this->PC;
        logger::error(ss.str());
        this->state = CPU_State::HALTED;
        return 0x00;  // Return BRK instruction to halt the CPU
    }

    this->RWB = 1;                      // Set R/W to high for read operation
    this->bus.write_address(this->PC);  // Write the address to be read to the bus

    // Use the address decoder to fetch data from the appropriate memory module
    byte data = 0;
    if (decoder_ptr) {
        try {
            data = decoder_ptr->read(this->PC);  // Read using the address decoder
        } catch (const std::exception& e) {
            std::stringstream ss;
            ss << "Error reading from address 0x" << std::hex << std::setfill('0') << std::setw(4) << this->PC << ": "
               << e.what();
            logger::error(ss.str());
            this->state = CPU_State::HALTED;
            return 0x00;  // Return BRK instruction to halt the CPU
        }
    } else {
        data = this->bus.read_data();  // Fallback to bus if no decoder
    }

    this->PC++;   // Increment program counter after reading
    return data;  // Return the read byte
}

word WDC65C02::fetch_word() {
    // Check for invalid PC value (should never happen with 16-bit address)
    if (this->PC > 0xFFFE) {  // Can't read word at 0xFFFF (only byte)
        std::stringstream ss;
        ss << "Cannot read word at address: PC=0x" << std::hex << std::setfill('0') << std::setw(4) << this->PC
           << " (end of memory)";
        logger::error(ss.str());
        this->state = CPU_State::HALTED;
        return 0x0000;
    }

    word old_pc = this->PC;  // Store the old program counter

    // Fetch low byte
    byte lo = fetch_byte();  // Use fetch_byte to get the low byte

    // Fetch high byte
    byte hi = fetch_byte();  // Use fetch_byte to get the high byte

    this->PC = old_pc + 2;  // Update PC correctly after reading two bytes
    return (hi << 8) | lo;  // Combine high and low byte to form the word
}

void WDC65C02::boot() {
    this->state = CPU_State::POWER_ON;  // Set the CPU state to POWER_ON
    this->reset();                      // Call reset to initialize the CPU
    this->state = CPU_State::RUNNING;   // Set the CPU state to RUNNING
}

// Set the value of the specified register
void WDC65C02::set(const Register r, const byte val) {
    this->registers[static_cast<byte>(r)] = val;
}

// Get the value of the specified register
const byte WDC65C02::get(const Register r) {
    return this->registers[static_cast<byte>(r)];
}

// Static variables for thread management
static std::thread cpu_thread;
static std::atomic<bool> cpu_running{false};

// Execute a single instruction
void WDC65C02::execute_instruction() {
    if (state != CPU_State::RUNNING) {
        return;
    }

    static bool waiting_for_clock_low = false;
    static bool instruction_complete = true;

    // State machine for instruction execution synchronized with clock
    if (this->PHI0 == 1 && !waiting_for_clock_low) {
        // Clock is high, execute if we have a new instruction
        if (instruction_complete) {
            // Make sure bus has the current data
            this->bus.write_address(this->PC);

            // Fetch the opcode
            byte opcode = fetch_byte();
            // PC was decremented by fetch_byte()

            // Execute the opcode
            switch (opcode) {
                case static_cast<byte>(Op::NOP): {
                    // No Operation - just consume a cycle
                    break;
                }

                case static_cast<byte>(Op::LDA_IM): {
                    // Load Accumulator with Immediate value
                    byte value = fetch_byte();
                    A = value;
                    // Set flags
                    FLAGS_Z = (A == 0);
                    FLAGS_N = ((A & 0x80) != 0);
                    break;
                }

                case static_cast<byte>(Op::LDX_IM): {
                    // Load X Register with Immediate value
                    byte value = fetch_byte();
                    X = value;
                    // Set flags
                    FLAGS_Z = (X == 0);
                    FLAGS_N = ((X & 0x80) != 0);
                    break;
                }

                case static_cast<byte>(Op::LDY_IM): {
                    // Load Y Register with Immediate value
                    byte value = fetch_byte();
                    Y = value;
                    // Set flags
                    FLAGS_Z = (Y == 0);
                    FLAGS_N = ((Y & 0x80) != 0);
                    break;
                }

                case static_cast<byte>(Op::STA_ABS): {
                    // Store Accumulator to Absolute address
                    word addr = fetch_word();
                    if (bus.request_bus(BusOwner::CPU)) {
                        bus.write_address(addr);
                        bus.write_data(A);
                        this->RWB = 0;  // Set to write mode
                        // Signal that data is on the bus
                        this->SYNC = 0;
                        // Wait a cycle
                        this->RWB = 1;  // Reset back to read mode
                        this->SYNC = 1;
                        bus.release_bus(BusOwner::CPU);
                    } else {
                        logger::error("Failed to get bus access for STA_ABS");
                    }
                    break;
                }

                case static_cast<byte>(Op::LDA_AB): {
                    // Load Accumulator from Absolute address
                    word addr = fetch_word();
                    if (bus.request_bus(BusOwner::CPU)) {
                        bus.write_address(addr);
                        A = bus.read_data();
                        // Set flags
                        FLAGS_Z = (A == 0);
                        FLAGS_N = ((A & 0x80) != 0);
                        bus.release_bus(BusOwner::CPU);
                    } else {
                        logger::error("Failed to get bus access for LDA_AB");
                    }
                    break;
                }

                case static_cast<byte>(Op::INX): {
                    // Increment X Register
                    X++;
                    // Set flags
                    FLAGS_Z = (X == 0);
                    FLAGS_N = ((X & 0x80) != 0);
                    break;
                }

                case static_cast<byte>(Op::INY): {
                    // Increment Y Register
                    Y++;
                    // Set flags
                    FLAGS_Z = (Y == 0);
                    FLAGS_N = ((Y & 0x80) != 0);
                    break;
                }

                case static_cast<byte>(Op::DEX): {
                    // Decrement X Register
                    X--;
                    // Set flags
                    FLAGS_Z = (X == 0);
                    FLAGS_N = ((X & 0x80) != 0);
                    break;
                }

                case static_cast<byte>(Op::DEY): {
                    // Decrement Y Register
                    Y--;
                    // Set flags
                    FLAGS_Z = (Y == 0);
                    FLAGS_N = ((Y & 0x80) != 0);
                    break;
                }

                case static_cast<byte>(Op::TAX): {
                    // Transfer Accumulator to X
                    X = A;
                    // Set flags
                    FLAGS_Z = (X == 0);
                    FLAGS_N = ((X & 0x80) != 0);
                    break;
                }

                case static_cast<byte>(Op::TAY): {
                    // Transfer Accumulator to Y
                    Y = A;
                    // Set flags
                    FLAGS_Z = (Y == 0);
                    FLAGS_N = ((Y & 0x80) != 0);
                    break;
                }

                case static_cast<byte>(Op::TXA): {
                    // Transfer X to Accumulator
                    A = X;
                    // Set flags
                    FLAGS_Z = (A == 0);
                    FLAGS_N = ((A & 0x80) != 0);
                    break;
                }

                case static_cast<byte>(Op::TYA): {
                    // Transfer Y to Accumulator
                    A = Y;
                    // Set flags
                    FLAGS_Z = (A == 0);
                    FLAGS_N = ((A & 0x80) != 0);
                    break;
                }

                case 0x00:  // Handle BRK instruction
                    // Force interrupt
                    state = CPU_State::HALTED;
                    break;

                default: {
                    std::stringstream ss;
                    ss << "Unimplemented opcode: 0x" << std::hex << std::setfill('0') << std::setw(2) << (int)opcode
                       << " at PC=0x" << std::hex << std::setfill('0') << std::setw(4) << (PC - 1);
                    logger::error(ss.str());
                    state = CPU_State::HALTED;
                    break;
                }
            }

            // Mark that we need to wait for clock to go low before next instruction
            waiting_for_clock_low = true;
            instruction_complete = false;
        }
    } else if (this->PHI0 == 0 && waiting_for_clock_low) {
        // Clock is low, reset for next instruction
        waiting_for_clock_low = false;
        instruction_complete = true;
    }
}

// Start executing instructions in a separate thread
void WDC65C02::execute() {
    // Don't start if already running
    if (cpu_running.load()) {
        logger::info("CPU already running");
        return;
    }

    // Set running flag to true
    cpu_running.store(true);

    // Start the CPU in a separate thread
    cpu_thread = std::thread([this]() {
        logger::info("CPU execution thread started");

        while (cpu_running.load() && state != CPU_State::HALTED && state != CPU_State::POWER_OFF) {
            // Check if CPU is ready
            if (this->RDY == 0) {
                // CPU is not ready, wait
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            // Check for reset signal
            if (this->RESB == 0) {
                logger::info("CPU reset triggered");
                this->reset();
                // Wait for reset to be released
                while (this->RESB == 0 && cpu_running.load()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
                continue;
            }

            // Execute a single instruction
            execute_instruction();

            // Monitor PHI0 for changes (synchronize with clock)
            static bool last_phi0 = this->PHI0;
            if (this->PHI0 != last_phi0) {
                last_phi0 = this->PHI0;
                if (this->PHI0 == 1) {
                    this->PHI1O = 1;  // Output phase 1 clock
                    this->PHI2O = 0;
                } else {
                    this->PHI1O = 0;
                    this->PHI2O = 1;  // Output phase 2 clock
                }
            }

            // Reduce CPU usage with longer sleep
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        if (state == CPU_State::HALTED) {
            logger::info("CPU halted");
        } else if (state == CPU_State::POWER_OFF) {
            logger::info("CPU powered off");
        } else {
            logger::info("CPU execution thread stopped");
        }
    });

    // Detach the thread so it can run independently
    cpu_thread.detach();
}
