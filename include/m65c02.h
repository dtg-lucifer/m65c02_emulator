#ifndef M65C02_H
#define M65C02_H

#include "types.h"

class M65C02 {
   private:
    byte registers[3];  // Index registers

   public:
    word PC;  // Program counter register
    byte SP;  // Stack pointer register (stores the lower 8-bit)

    // Register references for easier access
    byte& A = registers[static_cast<byte>(Register::A)];
    byte& X = registers[static_cast<byte>(Register::X)];
    byte& Y = registers[static_cast<byte>(Register::Y)];

    union {
        byte FLAGS;  // Status flags byte
        struct {
            byte FLAGS_N : 1;  // Negative Flag (bit 7)
            byte FLAGS_V : 1;  // Overflow Flag (bit 6)
            byte FLAGS_U : 1;  // Unused/expansion (bit 5)
            byte FLAGS_B : 1;  // Break Flag (bit 4)
            byte FLAGS_D : 1;  // Decimal Mode Flag (bit 3)
            byte FLAGS_I : 1;  // Interrupt Disable Flag (bit 2)
            byte FLAGS_Z : 1;  // Zero Flag (bit 1)
            byte FLAGS_C : 1;  // Carry Flag (bit 0)
        };
    };

    // Pin layout for MOS 6502 with address/data bus access
    union {
        pinl_t PINS;  // Raw access to all pins at once

        // === Individual pin mapping ===
        struct {
            pinl_t VSS1 : 1;   // VSS   - Ground
            pinl_t RDY : 1;    // RDY   - Ready (halts CPU when low)
            pinl_t PHI1O : 1;  // PHI1O - Phase 1 clock output
            pinl_t IRQB : 1;   // IRQB  - Interrupt Request (active low)
            pinl_t NC1 : 1;    // NC    - Not connected
            pinl_t NMIB : 1;   // NMIB  - Non-Maskable Interrupt (active low)
            pinl_t SYNC : 1;   // SYNC  - Indicates opcode fetch
            pinl_t VCC : 1;    // VCC   - +5V power

            // Address bus (low half)
            pinl_t A0 : 1;   // Address line bit 0
            pinl_t A1 : 1;   // Address line bit 1
            pinl_t A2 : 1;   // Address line bit 2
            pinl_t A3 : 1;   // Address line bit 3
            pinl_t A4 : 1;   // Address line bit 4
            pinl_t A5 : 1;   // Address line bit 5
            pinl_t A6 : 1;   // Address line bit 6
            pinl_t A7 : 1;   // Address line bit 7
            pinl_t A8 : 1;   // Address line bit 8
            pinl_t A9 : 1;   // Address line bit 9
            pinl_t A10 : 1;  // Address line bit 10
            pinl_t A11 : 1;  // Address line bit 11

            pinl_t VSS2 : 1;  // VSS   - Ground (second ground at pin 21)

            // Address bus (high half)
            pinl_t A12 : 1;  // Address line bit 12
            pinl_t A13 : 1;  // Address line bit 13
            pinl_t A14 : 1;  // Address line bit 14
            pinl_t A15 : 1;  // Address line bit 15

            // Data bus
            pinl_t D7 : 1;  // Data line bit 7
            pinl_t D6 : 1;  // Data line bit 6
            pinl_t D5 : 1;  // Data line bit 5
            pinl_t D4 : 1;  // Data line bit 4
            pinl_t D3 : 1;  // Data line bit 3
            pinl_t D2 : 1;  // Data line bit 2
            pinl_t D1 : 1;  // Data line bit 1
            pinl_t D0 : 1;  // Data line bit 0

            pinl_t RWB : 1;    // Read (high) / Write (low)
            pinl_t NC2 : 1;    // NC
            pinl_t NC3 : 1;    // NC
            pinl_t PHI0 : 1;   // PHI0 - External clock input
            pinl_t S0 : 1;     // S0
            pinl_t PHI2O : 1;  // PHI2O - Phase 2 clock output
            pinl_t RESB : 1;   // Reset (active low)
        };

        // === Packed access to buses ===
        struct {
            pinl_t _skipA0 : 8;  // Skip non-address pins before A0
            pinl_t ADDR : 16;    // A0..A15 (low to high bit order)
        } addr_bus;

        struct {
            pinl_t _skipD0 : 24;  // Skip pins before D7
            pinl_t DATA : 8;      // D7..D0
        } data_bus;
    };

    // Get the value of a register
    const byte get(const Register r);
    // Set the value of a register
    void set(const Register r, const byte val);

    // Reset the CPU and make it return to its
    // original state
    //
    // This will be called when the CPU is powered on
    // but you can call this to manually reset the CPU
    // while its powered on
    void reset();

    // To power-on the cpu
    //
    // this will call the reset function manually everytime
    // this is called too.
    void start();

    // To fetch the next 16-bits from the memory
    // (consumes cycles)
    word fetch_word();
    // To fetch the next 8-bits from the memory
    // (consumes cycles)
    byte featch_byte();

    // To read the next 16-bits
    // (doesn't consume cycles)
    word read_word();
    // To read the next 8-bits
    // (doesn't consume cycles)
    byte read_byte();

    // Always return the SP added with the page value
    // because the SP only stores the lower 8 bit of the
    // current address space
    word get_sp() const { return static_cast<word>(SP + 0x0100); }
};

#endif  // M65C02 CPU interface
