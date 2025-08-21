#include "wdc65c02.h"

#include "bus.h"

WDC65C02::WDC65C02(Bus& bus) : bus(bus) {
    // Clear registers
    for (int i = 0; i < 3; ++i) registers[i] = 0;

    PC = 0x0000;   // Will be set by reset() from vector
    SP = 0xFF;     // Stack starts at top of page 1
    FLAGS = 0x34;  // Default reset state (IRQ disabled, U=1)

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
    this->PC = 0xFFFC;  // Program counter starts at address 0

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
    this->RWB = 1;                      // Set R/W to high for read operation
    this->bus.write_address(this->PC);  // write the address to be read
    byte lo = this->bus.read_data();    // read the low byte
    this->PC++;                         // Increment program counter after reading
    byte hi = this->bus.read_data();    // read the high byte
    this->PC = ((hi << 8) | lo);        // Combine high and low byte to form the address

    // return to the old state
    this->state = old_state;  // Restore the previous state
}

void WDC65C02::attach_to_bus(Bus& bus) {
    this->bus = bus;  // Attach the bus reference
}

byte WDC65C02::read_byte() {
    this->RWB = 1;                      // Set R/W to high for read operation
    this->bus.write_address(this->PC);  // Write the address to be read to the bus
    byte data = this->bus.read_data();  // Read data from the bus
    return data;                        // Return the read byte
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
    this->RWB = 1;                      // Set R/W to high for read operation
    this->bus.write_address(this->PC);  // Write the address to be read to the bus
    byte data = this->bus.read_data();  // Read data from the bus
    this->PC++;                         // Increment program counter after reading
    return data;                        // Return the read byte
}

word WDC65C02::fetch_word() {
    word old_pc = this->PC;             // Store the old program counter
    this->RWB = 1;                      // Set R/W to high for read operation
    this->bus.write_address(this->PC);  // Write the address to be read to the bus
    byte lo = this->bus.read_data();    // Read the low byte
    this->PC++;                         // Increment program counter after reading
    byte hi = this->bus.read_data();    // Read the high byte
    this->PC++;                         // Increment program counter after reading
    this->PC = old_pc;                  // Restore the program counter to the old value
    return (hi << 8) | lo;              // Combine high and low byte to form the word
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
