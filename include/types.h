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

#endif  // TYPES_H
