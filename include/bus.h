#ifndef BUS_H
#define BUS_H

#include <cstdint>
#include <vector>

#include "types.h"

// The first 16-bits (`WORD_1`) will be reserved for the address lines
// and like that the next 8-bits (`BYTE_3`) are for the data lines
// the rest is flexible in use
class Bus {
   private:
    bool power;
    uint8_t width;
    std::vector<pinl_t> lines;  // Storage for bus lines when width > 32 bits

   public:
    // Creates a variable width bus
    Bus(uint8_t width) : power(true), width(width) {}
    Bus() : power(true), width(32) {}

    // For bus widths up to 32 bits, we can use a bitfield approach
    union {
        pinl_t PINS;  // Raw access to all pins at once (up to 32 bits)

        // Dynamically access individual bits (up to 32)
        struct {
            pinl_t PIN_01 : 1;
            pinl_t PIN_02 : 1;
            pinl_t PIN_03 : 1;
            pinl_t PIN_04 : 1;
            pinl_t PIN_05 : 1;
            pinl_t PIN_06 : 1;
            pinl_t PIN_07 : 1;
            pinl_t PIN_08 : 1;
            pinl_t PIN_09 : 1;
            pinl_t PIN_10 : 1;
            pinl_t PIN_11 : 1;
            pinl_t PIN_12 : 1;
            pinl_t PIN_13 : 1;
            pinl_t PIN_14 : 1;
            pinl_t PIN_15 : 1;
            pinl_t PIN_16 : 1;
            pinl_t PIN_17 : 1;
            pinl_t PIN_18 : 1;
            pinl_t PIN_19 : 1;
            pinl_t PIN_20 : 1;
            pinl_t PIN_21 : 1;
            pinl_t PIN_22 : 1;
            pinl_t PIN_23 : 1;
            pinl_t PIN_24 : 1;
            pinl_t PIN_25 : 1;
            pinl_t PIN_26 : 1;
            pinl_t PIN_27 : 1;
            pinl_t PIN_28 : 1;
            pinl_t PIN_29 : 1;
            pinl_t PIN_30 : 1;
            pinl_t PIN_31 : 1;
            pinl_t PIN_32 : 1;
        };

        // Access to 8-bit segments
        struct {
            byte BYTE_1 : 8;  // Bits 0-7
            byte BYTE_2 : 8;  // Bits 8-15
            byte BYTE_3 : 8;  // Bits 16-23
            byte BYTE_4 : 8;  // Bits 24-31
        } bytes;

        // Access to 16-bit segments
        struct {
            word WORD_1 : 16;  // Bits 0-15
            word WORD_2 : 16;  // Bits 16-31
        } words;

        // Get the address
        struct {
            word ADDR : 16;  // The first 16-bits
        };

        // Get the data
        struct {
            pinl_t __skip : 16;  // skip the address line
            byte DATA : 8;       // The next 8-bits for data
        };
    };

    // Set a specific pin value (works for any width bus)
    void set_pin(uint8_t pin_number, bool value) {
        if (pin_number < width) {
            // Set directly in the bitfield for first 32 pins
            pinl_t mask = 1UL << pin_number;
            if (value)
                PINS |= mask;  // Set the value if the value exists
            else
                PINS &= ~mask;  // If the value is 0 then toggle the value
        }
    }

    // Get a specific pin value (works for any width bus)
    bool get_pin(uint8_t pin_number) const {
        if (pin_number < width) {
            // Get directly from bitfield for first 32 pins
            pinl_t mask = 1UL << pin_number;
            return (PINS & mask) != 0;
        }
        return false;  // Out of range
    }

    // Get the bus width
    uint8_t get_width() const { return width; }

    // Check if the bus is powered
    bool is_powered() const { return power; }

    // Power control
    void power_on() { power = true; }
    void power_off() { power = false; }

    // Reset all pins to 0
    void reset() { PINS = 0; }

    // write the address line
    void write_address(word addr) {
        this->ADDR = addr & 0xFFFF;  // Write lower 16 bits
    }
    // read the address line
    word read_address() const {
        return this->ADDR;  // Read the address from the first 16-bits
    }

    // write in the data line
    void write_data(byte data) {
        this->DATA = data & 0x00FF;  // Write to the first byte
    }
    // read the data line
    byte read_data() const {
        return this->DATA;  // Read the data from the first byte
    }
};

#endif
