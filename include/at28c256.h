#ifndef AT28C256_H
#define AT28C256_H

#include "bus.h"
#include "types.h"

class AT28C256 {
   private:
    byte memory[32 * 1024];  // 32KB
    Bus& bus;

   public:
    AT28C256(Bus& bus) : bus(bus) {
        // Initialize memory to 0xFF (unprogrammed state)
        for (int i = 0; i < 32768; ++i) {
            memory[i] = 0xFF;
        }
    }

    union {
        pinl_t PINS;  // Raw access to all pins at once
        struct {
            pinl_t A_14 : 1;  // Address line bit 14
            pinl_t A_12 : 1;  // Address line bit 12
            pinl_t A_7 : 1;   // Address line bit 7
            pinl_t A_6 : 1;   // Address line bit 6
            pinl_t A_5 : 1;   // Address line bit 5
            pinl_t A_4 : 1;   // Address line bit 4
            pinl_t A_3 : 1;   // Address line bit 3
            pinl_t A_2 : 1;   // Address line bit 2
            pinl_t A_1 : 1;   // Address line bit 1
            pinl_t A_0 : 1;   // Address line bit 0

            pinl_t IO_0 : 1;  // Data line bit 0
            pinl_t IO_1 : 1;  // Data line bit 1
            pinl_t IO_2 : 1;  // Data line bit 2
            pinl_t GRND : 1;  // GRND

            pinl_t VCC : 1;   // VCC
            pinl_t WE : 1;    // Write Enable (active low)
            pinl_t A_13 : 1;  // Address line bit 13
            pinl_t A_8 : 1;   // Address line bit 8
            pinl_t A_9 : 1;   // Address line bit 9
            pinl_t A_11 : 1;  // Address line bit 11
            pinl_t OE : 1;    // Output Enable (active low)
            pinl_t A_10 : 1;  // Address line bit 10
            pinl_t CE : 1;    // Chip Enable (active low)

            pinl_t IO_7 : 1;  // Data line bit 7
            pinl_t IO_6 : 1;  // Data line bit 6
            pinl_t IO_5 : 1;  // Data line bit 5
            pinl_t IO_4 : 1;  // Data line bit 4
            pinl_t IO_3 : 1;  // Data line bit 3
        };
    };

    // Read a byte from the EEPROM at the specified address
    byte read_byte(word addr);
    // Write a byte to the EEPROM at the specified address
    void write_byte(word addr, byte data);
    // Read a word from the EEPROM at the specified address
    word read_word(word addr);
    // Write a word to the EEPROM at the specified address
    void write_word(word addr, word data);

    // Read the value from the bus and write it to the address
    // on the bus
    void read_from_bus();
    // Write the value from the address on the bus to the bus
    // This will write the data to the bus if the write enable
    // and chip enable are active
    void write_to_bus();
};

#endif  // AT28C256 EEPROM interface
