#include "at28c256.h"

byte AT28C256::read_byte(word addr) {
    // Check if address is valid
    if (addr >= 32 * 1024) {
        // Return 0xFF for invalid addresses (unprogrammed state)
        return 0xFF;
    }

    return memory[addr];
}

void AT28C256::write_byte(word addr, byte data) {
    // Check if address is valid
    if (addr >= 32 * 1024) {
        return;
    }

    // In a real AT28C256, writes take time and there's a software data protection mechanism
    // For simulation purposes, we'll just write directly
    memory[addr] = data;
}

word AT28C256::read_word(word addr) {
    // Read two consecutive bytes (little-endian format)
    byte low = read_byte(addr);
    byte high = read_byte(addr + 1);

    // Combine into a word
    return static_cast<word>(low) | (static_cast<word>(high) << 8);
}

void AT28C256::write_word(word addr, word data) {
    // Write two consecutive bytes (little-endian format)
    write_byte(addr, data & 0xFF);             // Low byte
    write_byte(addr + 1, (data >> 8) & 0xFF);  // High byte
}

void AT28C256::read_from_bus() {
    // Assemble address from address pins
    word address = 0;
    address |= A_0 ? 0x0001 : 0;
    address |= A_1 ? 0x0002 : 0;
    address |= A_2 ? 0x0004 : 0;
    address |= A_3 ? 0x0008 : 0;
    address |= A_4 ? 0x0010 : 0;
    address |= A_5 ? 0x0020 : 0;
    address |= A_6 ? 0x0040 : 0;
    address |= A_7 ? 0x0080 : 0;
    address |= A_8 ? 0x0100 : 0;
    address |= A_9 ? 0x0200 : 0;
    address |= A_10 ? 0x0400 : 0;
    address |= A_11 ? 0x0800 : 0;
    address |= A_12 ? 0x1000 : 0;
    address |= A_13 ? 0x2000 : 0;
    address |= A_14 ? 0x4000 : 0;

    // Get data from bus
    byte data = 0;
    data |= bus.bytes.BYTE_3 & 0xFF;  // Get data byte from bus

    // Check if write is enabled (active low)
    if (!WE && !CE) {
        // Write to memory
        memory[address] = data;
    }
}

void AT28C256::write_to_bus() {
    // Assemble address from address pins
    word address = 0;
    address |= A_0 ? 0x0001 : 0;
    address |= A_1 ? 0x0002 : 0;
    address |= A_2 ? 0x0004 : 0;
    address |= A_3 ? 0x0008 : 0;
    address |= A_4 ? 0x0010 : 0;
    address |= A_5 ? 0x0020 : 0;
    address |= A_6 ? 0x0040 : 0;
    address |= A_7 ? 0x0080 : 0;
    address |= A_8 ? 0x0100 : 0;
    address |= A_9 ? 0x0200 : 0;
    address |= A_10 ? 0x0400 : 0;
    address |= A_11 ? 0x0800 : 0;
    address |= A_12 ? 0x1000 : 0;
    address |= A_13 ? 0x2000 : 0;
    address |= A_14 ? 0x4000 : 0;

    // If output enable and chip enable are active (low), put data on bus
    if (!OE && !CE) {
        // Read memory and put on data lines
        byte data = memory[address];

        // Write to data pins
        IO_0 = (data & 0x01) != 0;
        IO_1 = (data & 0x02) != 0;
        IO_2 = (data & 0x04) != 0;
        IO_3 = (data & 0x08) != 0;
        IO_4 = (data & 0x10) != 0;
        IO_5 = (data & 0x20) != 0;
        IO_6 = (data & 0x40) != 0;
        IO_7 = (data & 0x80) != 0;

        // Update bus data lines
        bus.bytes.BYTE_3 = data;
    }
}
