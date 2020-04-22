#pragma once
#include <SDL2/SDL.h>
#include "debug.hpp"
#include "byte.hpp"
#include "counter.hpp"
#include "ines.hpp"
#include "2A03.hpp"

class Nes {
    //TODO: PPU
    private:
        Cpu cpu;
        Apu apu;
        
    public:
        Nes(SDL_AudioDeviceID& audioDevice) 
              : apu{cpu, audioDevice} {
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

            cpu.timer.reload = 11;
            apu.timer.reload = 11;
        }
        
        template <typename RomType>
        void load(RomType& rom) {
            MappedMemory<> dummyPpuMemory(0xFFFF);
            ines::load(rom, cpu.memory, dummyPpuMemory); 
            cpu.reset();
        }

        void tick(const u8_fast count = 1) {
            cpu.tick(count);
            apu.tick(count);
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
