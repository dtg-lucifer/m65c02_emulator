#ifndef DECODER_H
#define DECODER_H

#include <iostream>
#include <vector>

#include "i_memory.h"
#include "types.h"

struct Mapping {
    word start;
    word end;
    MEM_Module* module;
};

class AddressDecoder {
    std::vector<Mapping> map;

   public:
    void addMapping(word start, word end, MEM_Module* module) { map.push_back({start, end, module}); }

    byte read(word addr) {
        for (auto& m : map) {
            if (addr >= m.start && addr <= m.end) {
                return m.module->read_word(addr - m.start);
            }
        }
        std::cerr << "Invalid read at " << std::hex << addr << "\n";
        return 0xFF;
    }

    void write(word addr, word val) {
        for (auto& m : map) {
            if (addr >= m.start && addr <= m.end) {
                m.module->write_word(addr - m.start, val);
                return;
            }
        }
        std::cerr << "Invalid write at " << std::hex << addr << "\n";
    }
};

#endif  // DECODER_H
