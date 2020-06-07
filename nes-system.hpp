#pragma once
#include <functional>
#include "debug.hpp"
#include "byte.hpp"
#include "counter.hpp"
#include "ines.hpp"
#include "2A03.hpp"
#include "2C02.hpp"

class Nes {
    //TODO: PPU
    private:
        Cpu cpu;
        Apu apu {cpu};
        Ppu ppu {cpu};
        
    public:
        std::function<void(u8 sample)>& audioOutputFunction {
                apu.outputFunction};
        std::function<void(
                u8_fast x, 
                u8_fast y, 
                u32 pixel)>& videoOutputFunction {
                ppu.outputFunction};

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


            cpu.timer.reload = 11;
            apu.timer.reload = 11;
            ppu.timer.reload = 3;
        }
        
        template <typename RomType>
        void load(RomType& rom) {
            ines::load(rom, cpu.memory, ppu.memory); 
        }

        void reset() {
            cpu.reset();
            apu.reset();
            ppu.reset();
        }

        void tick(const u8_fast ticks = 1) {
            cpu.tick(ticks);
            apu.tick(ticks);
            ppu.tick(ticks);
        }

        void ramdump(const char* const filename) {
            std::ofstream ramdumpFile {filename,
                    std::ofstream::binary | std::ofstream::trunc};
            auto ptr {ppu.memory.begin()};
            for (u32_fast i {0}; i <= ppu.memory.memory.size(); ++i, ++ptr) {
                u8 byte {*ptr};
                ramdumpFile.write(reinterpret_cast<char*>(&byte), 1);
            }
            ptr = cpu.memory.begin();
            for (u32_fast i {0}; i <= cpu.memory.memory.size(); ++i, ++ptr) {
                u8 byte {*ptr};
                ramdumpFile.write(reinterpret_cast<char*>(&byte), 1);
            }
            ramdumpFile.close();
        }
};
