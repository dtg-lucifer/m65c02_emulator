#ifndef HM62256B_H
#define HM62256B_H

#include "bus.h"
#include "memory.h"
#include "types.h"

class HM62256B : public MEM_Module {
   public:                   // Make memory public for debugging purposes
    byte memory[32 * 1024];  // 32KB of SRAM
   private:
    Bus& bus;

   public:
    HM62256B(Bus& bus) : bus(bus) {
        // Initialize memory to 0x00 (cleared state)
        for (int i = 0; i < 32768; ++i) {
            memory[i] = 0x00;
        }
    }

    // Pin layout for HM62256B SRAM
    union {
        pinl_t PINS;  // Raw access to all pins at once
        struct {
            pinl_t A14 : 1;  // Address line bit 14
            pinl_t A12 : 1;  // Address line bit 12
            pinl_t A7 : 1;   // Address line bit 7
            pinl_t A6 : 1;   // Address line bit 6
            pinl_t A5 : 1;   // Address line bit 5
            pinl_t A4 : 1;   // Address line bit 4
            pinl_t A3 : 1;   // Address line bit 3
            pinl_t A2 : 1;   // Address line bit 2
            pinl_t A1 : 1;   // Address line bit 1
            pinl_t A0 : 1;   // Address line bit 0
            pinl_t IO0 : 1;  // Data line bit 0
            pinl_t IO1 : 1;  // Data line bit 1
            pinl_t IO2 : 1;  // Data line bit 2
            pinl_t VSS : 1;  // Ground

            pinl_t VCC : 1;  // VCC
            pinl_t WE : 1;   // Write Enable (active low)
            pinl_t A13 : 1;  // Address line bit 13
            pinl_t A8 : 1;   // Address line bit 8
            pinl_t A9 : 1;   // Address line bit 9
            pinl_t A11 : 1;  // Address line bit 11
            pinl_t OE : 1;   // Output Enable (active low)
            pinl_t A10 : 1;  // Address line bit 10
            pinl_t CS : 1;   // Chip Select (active low)

            pinl_t IO7 : 1;  // Data line bit 7
            pinl_t IO6 : 1;  // Data line bit 6
            pinl_t IO5 : 1;  // Data line bit 5
            pinl_t IO4 : 1;  // Data line bit 4
            pinl_t IO3 : 1;  // Data line bit 3
        };
    };

    // Read the value from the bus and write it to the address
    // on the bus
    void read_from_bus();

    // Write the value from the address on the bus to the bus
    // This will write the data to the bus if the output enable
    // and chip select are active
    void write_to_bus();

    // Attach bus
    void attach_to_bus(Bus& bus);

    // Start monitoring the bus in a separate thread
    void start_monitoring();

    // Stop monitoring the bus
    void stop_monitoring();

    // Memory interface implementation
    word read_word(word addr) override;
    byte read_byte(byte addr) override;
    void write_word(word addr, word data) override;
    void write_byte(byte addr, byte data) override;
};

#endif  // HM62256B SRAM interface
