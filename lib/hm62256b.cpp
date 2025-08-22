#include "hm62256b.h"

#include <atomic>
#include <chrono>
#include <thread>

#include "log.h"

// Static thread management variables
static std::thread sram_thread;
static std::atomic<bool> sram_running{false};

void HM62256B::read_from_bus() {
    // Check if chip is selected (CS is active low)
    if (CS != 0) return;

    // Check if write is enabled (WE is active low)
    if (WE != 0) return;

    // Construct the address from individual address pins
    word address = 0;
    address |= (A0 ? 1 : 0);
    address |= (A1 ? 1 : 0) << 1;
    address |= (A2 ? 1 : 0) << 2;
    address |= (A3 ? 1 : 0) << 3;
    address |= (A4 ? 1 : 0) << 4;
    address |= (A5 ? 1 : 0) << 5;
    address |= (A6 ? 1 : 0) << 6;
    address |= (A7 ? 1 : 0) << 7;
    address |= (A8 ? 1 : 0) << 8;
    address |= (A9 ? 1 : 0) << 9;
    address |= (A10 ? 1 : 0) << 10;
    address |= (A11 ? 1 : 0) << 11;
    address |= (A12 ? 1 : 0) << 12;
    address |= (A13 ? 1 : 0) << 13;
    address |= (A14 ? 1 : 0) << 14;

    // Ensure address is within bounds
    if (address >= 32 * 1024) return;

    // Get the data from the bus
    byte data = 0;
    data |= (IO0 ? 1 : 0);
    data |= (IO1 ? 1 : 0) << 1;
    data |= (IO2 ? 1 : 0) << 2;
    data |= (IO3 ? 1 : 0) << 3;
    data |= (IO4 ? 1 : 0) << 4;
    data |= (IO5 ? 1 : 0) << 5;
    data |= (IO6 ? 1 : 0) << 6;
    data |= (IO7 ? 1 : 0) << 7;

    // Store the data in memory
    memory[address] = data;
}

void HM62256B::write_to_bus() {
    // Check if chip is selected (CS is active low)
    if (CS != 0) return;

    // Check if output is enabled (OE is active low for reading)
    if (OE != 0) return;

    // For writing, WE should be high
    if (WE == 0) return;

    // Construct the address from individual address pins
    word address = 0;
    address |= (A0 ? 1 : 0);
    address |= (A1 ? 1 : 0) << 1;
    address |= (A2 ? 1 : 0) << 2;
    address |= (A3 ? 1 : 0) << 3;
    address |= (A4 ? 1 : 0) << 4;
    address |= (A5 ? 1 : 0) << 5;
    address |= (A6 ? 1 : 0) << 6;
    address |= (A7 ? 1 : 0) << 7;
    address |= (A8 ? 1 : 0) << 8;
    address |= (A9 ? 1 : 0) << 9;
    address |= (A10 ? 1 : 0) << 10;
    address |= (A11 ? 1 : 0) << 11;
    address |= (A12 ? 1 : 0) << 12;
    address |= (A13 ? 1 : 0) << 13;
    address |= (A14 ? 1 : 0) << 14;

    // Ensure address is within bounds
    if (address >= 32 * 1024) return;

    // Get data from memory
    byte data = memory[address];

    // Set the data pins
    IO0 = (data & 0x01) != 0;
    IO1 = (data & 0x02) != 0;
    IO2 = (data & 0x04) != 0;
    IO3 = (data & 0x08) != 0;
    IO4 = (data & 0x10) != 0;
    IO5 = (data & 0x20) != 0;
    IO6 = (data & 0x40) != 0;
    IO7 = (data & 0x80) != 0;

    // Request the bus to update its data lines
    if (bus.request_bus(BusOwner::MEMORY)) {
        // Copy our data to the bus
        bus.write_data(data);
        bus.release_bus(BusOwner::MEMORY);
    }
}

// Implement MEM_Module interface methods
word HM62256B::read_word(word addr) {
    if (addr >= 32 * 1024) {
        return 0xFFFF;  // Out of bounds
    }
    return memory[addr];
}

byte HM62256B::read_byte(byte addr) {
    if (addr >= 32 * 1024) {
        return 0xFF;  // Out of bounds
    }
    return memory[addr];
}

void HM62256B::write_word(word addr, word data) {
    if (addr >= 32 * 1024) {
        return;  // Out of bounds
    }
    memory[addr] = data & 0xFF;  // Only write the lower 8 bits
}

void HM62256B::write_byte(byte addr, byte data) {
    if (addr >= 256) {
        return;  // Out of bounds
    }
    memory[addr] = data;
}

void HM62256B::attach_to_bus(Bus& new_bus) {
    this->bus = new_bus;
}

// Start the SRAM monitoring thread
void HM62256B::start_monitoring() {
    // Don't start if already running
    if (sram_running.load()) {
        return;
    }

    // Set running flag to true
    sram_running.store(true);

    // Start the SRAM monitor in a separate thread
    sram_thread = std::thread([this]() {
        logger::info("HM62256B SRAM started monitoring bus");

        // Track previous pin states to detect changes
        bool prev_cs = CS;
        bool prev_we = WE;
        bool prev_oe = OE;

        while (sram_running.load()) {
            // Only process if pins have changed or chip is active
            bool pins_changed = (prev_cs != CS || prev_we != WE || prev_oe != OE);
            (void)pins_changed;  // Avoid unused variable warning

            // Update previous pin states
            prev_cs = CS;
            prev_we = WE;
            prev_oe = OE;

            // Monitor the bus for read/write operations only when needed
            if (CS == 0) {  // Chip selected
                if (WE == 0) {
                    // Write operation (SRAM is being written to)
                    this->read_from_bus();
                } else if (OE == 0) {
                    // Read operation (SRAM is being read from)
                    this->write_to_bus();
                }
            }

            // Adaptive sleep - sleep longer if nothing is happening
            if (CS != 0 || (WE != 0 && OE != 0)) {
                // Chip not active or no operation in progress, can sleep longer
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            } else {
                // Chip active and operation in progress, need to respond quickly
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }

        logger::info("HM62256B SRAM monitor thread stopped");
    });

    // Detach the thread so it can run independently
    sram_thread.detach();
}

// Stop the SRAM monitoring thread
void HM62256B::stop_monitoring() {
    sram_running.store(false);
}
