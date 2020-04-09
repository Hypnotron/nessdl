#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>
#include "byte.hpp"
#include "memory.hpp"
#include "2A03.hpp"

int main(int argc, char* argv[]) {
    Cpu cpu {};
    
    std::ifstream nestestFile {"15-brk.nes", 
            std::ofstream::binary | std::ofstream::in};
    nestestFile.seekg(0x10, std::ios::beg);
    nestestFile.read(reinterpret_cast<char*>(cpu.memory.memory.data() + 0x8000), 0x8000);
    nestestFile.close(); 
    
    cpu.memory.readFunctions[0x1FFF] = [] (MappedMemory<>* const memory, const u16 address) {
        return memory->memory[address & 0x07FF];
    };
    cpu.memory.readFunctions[0xFFFF] = [] (MappedMemory<>* const memory, const u16 address) {
        return memory->memory[address];
    };
    cpu.memory.writeFunctions[0x1FFF] = [] (MappedMemory<>* const memory, const u16 address, const u8 data) {
        memory->memory[address & 0x07FF] = data;
    };
    cpu.memory.writeFunctions[0xFFFF] = [] (MappedMemory<>* const memory, const u16 address, const u8 data) {
        memory->memory[address] = data;
    };

    cpu.reset();
    for (u64_fast i = 0; i < 10000000; ++i) {
        cpu.tick();
    }

    std::ofstream dumpFile {"ram.dump",
            std::ofstream::binary | std::ofstream::trunc};
    dumpFile.write(reinterpret_cast<char*>(cpu.memory.memory.data()), 0xFFFF);
    dumpFile.close();
    
    
    return 0;
} 
