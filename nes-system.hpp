#pragma once
#include <functional>
//TODO: remove
#include <iostream>
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
        
        bool controllerStrobe {false};
        u8_fast controller1Shift {0}, controller2Shift {0};

    public:
        std::function<void(u8 sample)>& audioOutputFunction {
                apu.outputFunction};
        std::function<void(
                u8_fast x, 
                u8_fast y, 
                u32 pixel)>& videoOutputFunction {
                ppu.outputFunction};
        u8_fast controller1 {0}, controller2 {0};

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
            cpu.memory.readFunctions[0x4016] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address) {
                bool result = controller1Shift & 0x01;
                controller1Shift >>= !controllerStrobe;
                return result;
            };
            cpu.memory.writeFunctions[0x4016] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                if (controllerStrobe || data & 0x01) {
                    controller1Shift = controller1;
                    controller2Shift = controller2;
                }
                controllerStrobe = data & 0x01;
            };
            cpu.memory.readFunctions[0x4017] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address) {
                bool result = controller2Shift & 0x01;
                controller2Shift >>= !controllerStrobe;
                return result;
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
