#ifndef TYPES_H
#define TYPES_H

#include <cstdint>

typedef uint16_t word;        // For 16 bit wide memory reference
typedef uint8_t byte;         // For 8 bit wide memory reference
typedef uint32_t pinl_t;      // For the CPU / EEPROM pin layout
typedef uint8_t clock_pin_t;  // For the clock pin layout

// Holds the location of each register
enum class Register : byte {
    A = 0,  // Accumulator
    X = 1,  // Index register X
    Y = 2,  // Index register Y
};

enum class ClockMode : byte {
    A_STABLE = 0x00,    // Continuous clock cycles
    MONO_STABLE = 0x01  // Manual stepping mode
};

// Holds the CPU state
enum class CPU_State : byte {
    POWER_OFF = 0x00,  // CPU is powered off
    POWER_ON = 0x01,   // CPU is powered on
    HALTED = 0x02,     // CPU is halted
    RUNNING = 0x03,    // CPU is running
    RESET = 0x04       // CPU is in reset state
};

#endif  // TYPES_H
