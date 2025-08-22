#include "at28c256.h"

#include <atomic>
#include <chrono>
#include <thread>

#include "log.h"

// Static thread management variables
static std::thread eeprom_thread;
static std::atomic<bool> eeprom_running{false};

void AT28C256::read_from_bus() {
    // Check if chip is enabled (CE is active low)
    if (CE != 0) return;

    // Check if write is enabled (WE is active low)
    if (WE != 0) return;

    // Check if output is enabled (OE should be active low for reading)
    if (OE != 0) return;

    // Construct the address from individual address pins
    word address = 0;
    address |= (A_0 ? 1 : 0);
    address |= (A_1 ? 1 : 0) << 1;
    address |= (A_2 ? 1 : 0) << 2;
    address |= (A_3 ? 1 : 0) << 3;
    address |= (A_4 ? 1 : 0) << 4;
    address |= (A_5 ? 1 : 0) << 5;
    address |= (A_6 ? 1 : 0) << 6;
    address |= (A_7 ? 1 : 0) << 7;
    address |= (A_8 ? 1 : 0) << 8;
    address |= (A_9 ? 1 : 0) << 9;
    address |= (A_10 ? 1 : 0) << 10;
    address |= (A_11 ? 1 : 0) << 11;
    address |= (A_12 ? 1 : 0) << 12;
    address |= (A_13 ? 1 : 0) << 13;
    address |= (A_14 ? 1 : 0) << 14;

    // Ensure address is within bounds
    if (address >= 32 * 1024) return;

    // Get the data from the bus
    byte data = 0;
    data |= (IO_0 ? 1 : 0);
    data |= (IO_1 ? 1 : 0) << 1;
    data |= (IO_2 ? 1 : 0) << 2;
    data |= (IO_3 ? 1 : 0) << 3;
    data |= (IO_4 ? 1 : 0) << 4;
    data |= (IO_5 ? 1 : 0) << 5;
    data |= (IO_6 ? 1 : 0) << 6;
    data |= (IO_7 ? 1 : 0) << 7;

    // Store the data in memory
    memory[address] = data;
}

void AT28C256::write_to_bus() {
    // Check if chip is enabled (CE is active low)
    if (CE != 0) return;

    // Check if output is enabled (OE is active low for reading)
    if (OE != 0) return;

    // For writing, WE should be high
    if (WE == 0) return;

    // Construct the address from individual address pins
    word address = 0;
    address |= (A_0 ? 1 : 0);
    address |= (A_1 ? 1 : 0) << 1;
    address |= (A_2 ? 1 : 0) << 2;
    address |= (A_3 ? 1 : 0) << 3;
    address |= (A_4 ? 1 : 0) << 4;
    address |= (A_5 ? 1 : 0) << 5;
    address |= (A_6 ? 1 : 0) << 6;
    address |= (A_7 ? 1 : 0) << 7;
    address |= (A_8 ? 1 : 0) << 8;
    address |= (A_9 ? 1 : 0) << 9;
    address |= (A_10 ? 1 : 0) << 10;
    address |= (A_11 ? 1 : 0) << 11;
    address |= (A_12 ? 1 : 0) << 12;
    address |= (A_13 ? 1 : 0) << 13;
    address |= (A_14 ? 1 : 0) << 14;

    // Ensure address is within bounds
    if (address >= 32 * 1024) return;

    // Get data from memory
    byte data = memory[address];

    // Set the data pins
    IO_0 = (data & 0x01) != 0;
    IO_1 = (data & 0x02) != 0;
    IO_2 = (data & 0x04) != 0;
    IO_3 = (data & 0x08) != 0;
    IO_4 = (data & 0x10) != 0;
    IO_5 = (data & 0x20) != 0;
    IO_6 = (data & 0x40) != 0;
    IO_7 = (data & 0x80) != 0;

    // Request the bus to update its data lines
    if (bus.request_bus(BusOwner::MEMORY)) {
        // Copy our data to the bus
        bus.write_data(data);
        bus.release_bus(BusOwner::MEMORY);
    }
}

// Implement MEM_Module interface methods
word AT28C256::read_word(word addr) {
    if (addr >= 32 * 1024) {
        return 0xFFFF;  // Out of bounds
    }
    return memory[addr];
}

byte AT28C256::read_byte(byte addr) {
    if (addr >= 32 * 1024U) {
        return 0xFF;  // Out of bounds
    }
    return memory[addr];
}

void AT28C256::write_word(word addr, word data) {
    if (addr >= 32 * 1024U) {
        return;  // Out of bounds
    }
    memory[addr] = data & 0xFF;  // Only write the lower 8 bits
}

void AT28C256::write_byte(byte addr, byte data) {
    if (addr >= 256U) {
        return;  // Out of bounds
    }
    memory[addr] = data;
}

void AT28C256::attach_to_bus(Bus& new_bus) {
    this->bus = new_bus;
}

// Start the EEPROM monitoring thread
void AT28C256::start_monitoring() {
    // Don't start if already running
    if (eeprom_running.load()) {
        // Silently ignore
        return;
    }

    // Set running flag to true
    eeprom_running.store(true);

    // Start the EEPROM monitor in a separate thread
    eeprom_thread = std::thread([this]() {
        logger::info("AT28C256 EEPROM started monitoring bus");

        // Track previous pin states to detect changes
        bool prev_ce = CE;
        bool prev_we = WE;
        bool prev_oe = OE;

        while (eeprom_running.load()) {
            // Only process if pins have changed or chip is active
            // Monitor for pin changes
            bool pins_changed = (prev_ce != CE || prev_we != WE || prev_oe != OE);
            (void)pins_changed;  // Avoid unused variable warning

            // Update previous pin states
            prev_ce = CE;
            prev_we = WE;
            prev_oe = OE;

            // Monitor the bus for read/write operations only when needed
            if (CE == 0) {  // Chip enabled
                if (WE == 0) {
                    // Write operation (EEPROM is being written to)
                    this->read_from_bus();
                } else if (OE == 0) {
                    // Read operation (EEPROM is being read from)
                    this->write_to_bus();
                }
            }

            // Adaptive sleep - sleep longer if nothing is happening
            if (CE != 0 || (WE != 0 && OE != 0)) {
                // Chip not active or no operation in progress, can sleep longer
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            } else {
                // Chip active and operation in progress, need to respond quickly
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }

        logger::info("AT28C256 EEPROM monitor thread stopped");
    });

    // Detach the thread so it can run independently
    eeprom_thread.detach();
}

// Stop the EEPROM monitoring thread
void AT28C256::stop_monitoring() {
    eeprom_running.store(false);
}
