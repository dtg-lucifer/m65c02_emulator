#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

class MEM_Module {
   public:
    virtual word read_word(word addr) = 0;
    virtual byte read_byte(byte addr) = 0;
    virtual void write_word(word addr, word data) = 0;
    virtual void write_byte(byte addr, byte data) = 0;
    virtual ~MEM_Module() = default;
};

#endif  // MEMORY_H
