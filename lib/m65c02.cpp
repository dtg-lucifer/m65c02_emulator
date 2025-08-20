#include "m65c02.h"

#include <iostream>

void M65C02::reset() {
    // Initialize registers
    A = 0;
    X = 0;
    Y = 0;

    // Initialize status flags
    FLAGS = 0;
    FLAGS_I = 1;  // Interrupt disable set on reset

    // Initialize stack pointer
    SP = 0xFF;

    // Set pins to default values
    PINS = 0;
    RDY = 1;   // Ready (active high)
    RESB = 1;  // Reset (active low)
    IRQB = 1;  // Interrupt Request (active low)
    NMIB = 1;  // Non-maskable Interrupt (active low)
    RWB = 1;   // Set to read mode

    // In a real 6502, the reset vector is fetched from 0xFFFC-0xFFFD
    // We'll simulate this process

    // First put address 0xFFFC on the bus
    addr_bus.ADDR = 0xFFFC;

    // Read the low byte from the bus
    bus.words.WORD_1 = 0xFFFC;  // Put the address on the bus
    bus.bytes.BYTE_4 = 1;       // Signal read operation

    byte low_byte = bus.bytes.BYTE_3;  // Read data from bus

    // Now read the high byte from 0xFFFD
    bus.words.WORD_1 = 0xFFFD;
    bus.bytes.BYTE_4 = 1;  // Signal read operation

    byte high_byte = bus.bytes.BYTE_3;  // Read data from bus

    // Combine to form the reset vector address
    PC = (static_cast<word>(high_byte) << 8) | low_byte;

    std::cout << "CPU reset. PC set to 0x" << std::hex << PC << std::endl;
}

void M65C02::start() {
    // Power on the CPU
    RESB = 0;  // Activate reset (active low)

    // Wait a moment (simulating power-on delay)

    // Release reset
    RESB = 1;

    // The reset function will be called by the main execution loop
    // when it detects the reset pin going from low to high
    reset();
}

word M65C02::fetch_word() {
    // Read next two bytes from memory (little-endian)
    word result = read_word();
    PC += 2;  // Increment PC by 2 for word read
    return result;
}

byte M65C02::featch_byte() {
    // Read next byte from memory
    byte result = read_byte();
    PC += 1;  // Increment PC by 1 for byte read
    return result;
}

word M65C02::read_word() {
    // Read word at PC without incrementing PC
    // Put address on address bus
    bus.words.WORD_1 = PC;

    // Set read mode
    bus.bytes.BYTE_4 = 1;  // Signal read operation

    // Read low byte
    byte low_byte = bus.bytes.BYTE_3;

    // Read high byte
    bus.words.WORD_1 = PC + 1;
    byte high_byte = bus.bytes.BYTE_3;

    // Return combined word (little-endian)
    return (static_cast<word>(high_byte) << 8) | low_byte;
}

byte M65C02::read_byte() {
    // Read byte at PC without incrementing PC
    // Put address on address bus
    bus.words.WORD_1 = PC;

    // Set read mode
    bus.bytes.BYTE_4 = 1;  // Signal read operation

    // Return data from bus
    return bus.bytes.BYTE_3;
}

const byte M65C02::get(const Register r) {
    return registers[static_cast<byte>(r)];
}

void M65C02::set(const Register r, const byte val) {
    registers[static_cast<byte>(r)] = val;

    // Update flags based on value
    if (r == Register::A) {
        // Update negative flag
        FLAGS_N = (val & 0x80) != 0;
        // Update zero flag
        FLAGS_Z = val == 0;
    }
}
