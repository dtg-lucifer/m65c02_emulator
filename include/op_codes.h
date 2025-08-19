#ifndef OPCODES_H
#define OPCODES_H

#include "types.h"

enum class Op : byte {
    BRK = 0x00,       // Force Interrupt (Break)
    NOP = 0xEA,       // No Operation
    LDA_IM = 0xA9,    // Load Accumulator Immediate
    LDA_AB = 0xAD,    // Load Accumulator Absolute
    LDA_ZP = 0xA5,    // Load Accumulator Zero Page
    LDA_ZPX = 0xB5,   // Load Accumulator Zero Page,X
    LDA_ABSX = 0xBD,  // Load Accumulator Absolute,X
    LDA_ABSY = 0xB9,  // Load Accumulator Absolute,Y
    LDA_INX = 0xA1,   // Load Accumulator (Indirect,X)
    LDA_INY = 0xB1,   // Load Accumulator (Indirect),Y
    // -----------------------------------------------
    LDX_IM = 0xA2,    // Load X Register Immediate
    LDX_ZP = 0xA6,    // Load X Register Zero Page
    LDX_ZPY = 0xB6,   // Load X Register Zero Page,Y
    LDX_AB = 0xAE,    // Load X Register Absolute
    LDX_ABSY = 0xBE,  // Load X Register Absolute,Y
    // -----------------------------------------------
    LDY_IM = 0xA0,    // Load Y Register Immediate
    LDY_ZP = 0xA4,    // Load Y Register Zero Page
    LDY_ZPX = 0xB4,   // Load Y Register Zero Page,X
    LDY_AB = 0xAC,    // Load Y Register Absolute
    LDY_ABSX = 0xBC,  // Load Y Register Absolute,X
    // -----------------------------------------------
    STA_ZP = 0x85,    // Store Accumulator Zero Page
    STA_ZPX = 0x95,   // Store Accumulator Zero Page,X
    STA_ABS = 0x8D,   // Store Accumulator Absolute
    STA_ABSX = 0x9D,  // Store Accumulator Absolute,X
    STA_ABSY = 0x99,  // Store Accumulator Absolute,Y
    STA_INX = 0x81,   // Store Accumulator (Indirect,X)
    STA_INY = 0x91,   // Store Accumulator (Indirect),Y
    // -----------------------------------------------
    STX_ZP = 0x86,   // Store X Register Zero Page
    STX_ZPY = 0x96,  // Store X Register Zero Page,Y
    STX_ABS = 0x8E,  // Store X Register Absolute
    // -----------------------------------------------
    STY_ZP = 0x84,   // Store Y Register Zero Page
    STY_ZPX = 0x94,  // Store Y Register Zero Page,X
    STY_ABS = 0x8C,  // Store Y Register Absolute
    // -----------------------------------------------
    JSR = 0x20,   // Jump to Subroutine
    RTS = 0x60,   // Return from Subroutine
    JMP = 0x4C,   // Jump Absolute
    JMPI = 0x6C,  // Jump Indirect
    // -----------------------------------------------
    // Stack Operations
    PHA = 0x48,  // Push Accumulator on Stack
    PHP = 0x08,  // Push Processor Status on Stack
    PLA = 0x68,  // Pull Accumulator from Stack
    PLP = 0x28,  // Pull Processor Status from Stack
    // -----------------------------------------------
    // Register Transfer Operations
    TSX = 0xBA,  // Transfer Stack Pointer to X
    TXS = 0x9A,  // Transfer X to Stack Pointer
    // -----------------------------------------------
};

#endif
