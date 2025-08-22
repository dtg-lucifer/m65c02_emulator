#ifndef DECODER_H
#define DECODER_H

#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include "log.h"
#include "memory.h"
#include "types.h"

struct Mapping {
    word start;
    word end;
    MEM_Module* module;
};

class AddressDecoder {
   private:
    std::vector<Mapping> map;

   public:
    void add_mapping(word start, word end, MEM_Module* module) { map.push_back({start, end, module}); }

    byte read(word addr) {
        for (auto& m : map) {
            if (addr >= m.start && addr <= m.end) {
                // Calculate local address within the module
                word local_addr = addr - m.start;
                return m.module->read_word(local_addr);
            }
        }
        // Use proper logging instead of cerr
        std::stringstream ss;
        ss << "Invalid memory read at address 0x" << std::hex << std::setw(4) << std::setfill('0') << addr;
        logger::error(ss.str());
        return 0xFF;  // Return a default value for unmapped memory
    }

    void write(word addr, word val) {
        for (auto& m : map) {
            if (addr >= m.start && addr <= m.end) {
                // Calculate local address within the module
                word local_addr = addr - m.start;
                m.module->write_word(local_addr, val);
                return;
            }
        }
        // Use proper logging instead of cerr
        std::stringstream ss;
        ss << "Invalid memory write at address 0x" << std::hex << std::setw(4) << std::setfill('0') << addr
           << " with value 0x" << std::setw(2) << std::setfill('0') << (int)val;
        logger::error(ss.str());
    }
};

#endif  // DECODER_H
