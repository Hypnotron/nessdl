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
        
    public:
        std::function<void(u8 sample)>& audioOutputFunction {
                apu.outputFunction};

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
            //TODO: PPU


            cpu.timer.reload = 11;
            apu.timer.reload = 11;
        }
        
        template <typename RomType>
        void load(RomType& rom) {
            MappedMemory<> dummyPpuMemory(0xFFFF);
            ines::load(rom, cpu.memory, dummyPpuMemory); 
        }

        void reset() {
            cpu.reset();
            apu.reset();
        }

        void tick(const u8_fast ticks = 1) {
            cpu.tick(ticks);
            apu.tick(ticks);
        }

        void ramdump(const char* const filename) {
            std::ofstream ramdumpFile {filename,
                    std::ofstream::binary | std::ofstream::trunc};
            auto ptr {cpu.memory.begin()};
            for (u32_fast i {0}; i <= 0xFFFF; ++i, ++ptr) {
                u8 byte {*ptr};
                ramdumpFile.write(reinterpret_cast<char*>(&byte), 1);
            }
            ramdumpFile.close();
        }
};
