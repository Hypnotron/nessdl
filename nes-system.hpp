#pragma once
#include <string>
#include "debug.hpp"
#include "counter.hpp"
#include "ines.hpp"
#include "2A03.hpp"

class Nes {
    //TODO: PPU
    private:
        Cpu cpu;
        Apu apu{cpu};
        
        Counter<s8_fast> cpuClock{11, [&] () {
            cpu.tick();
            apu.tick();
        }};

    public:
        Nes() {
            cpu.memory.readFunctions[0x1FFF] = [] (
                    MappedMemory<>* const memory, 
                    const u16 address) {
                return memory->memory[address & 0x07FF];
            };
            cpu.memory.writeFunctions[0x1FFF] = [] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                memory->memory[address & 0x07FF] = data;
            };
            //TODO: remove
            cpu.memory.readFunctions[0x3FFF] = [] (
                    MappedMemory<>* const memory,
                    const u16 address) {
                return 0;
            };
            cpu.memory.writeFunctions[0x3FFF] = [] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
            };
        }
        
        template <typename RomType>
        void load(RomType& rom) {
            MappedMemory<> dummyPpuMemory(0xFFFF);
            ines::load(rom, cpu.memory, dummyPpuMemory); 
            cpu.reset();
        }

        void tick() {
            cpuClock.tick();
        }

        //TODO: remove
        void ramdump() {
            std::ofstream ramdumpFile {"ram.dump",
                    std::ofstream::binary | std::ofstream::trunc};
            ramdumpFile.seekp(0, std::ios::beg);
            ramdumpFile.write(reinterpret_cast<char*>(cpu.memory.memory.data()), cpu.memory.memory.size());
            ramdumpFile.close();
        }
};
