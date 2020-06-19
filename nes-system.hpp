#pragma once
#include <functional>
#include "byte.hpp"
#include "counter.hpp"
#include "ines.hpp"
#include "2A03.hpp"
#include "2C02.hpp"

class Nes {
    private:
        Cpu cpu;
        Apu apu{cpu};
        Ppu ppu{cpu};
        Cartridge cart{cpu.memory, ppu.memory};
        
        u8_fast controller1Button {0}, controller2Button {0};
        bool controllerStrobe {false};

    public:
        std::function<void(u8 sample)>& audioOutputFunction {
                apu.outputFunction};
        std::function<void(
                u8_fast x, 
                u8_fast y, 
                u32 pixel)>& videoOutputFunction {
                ppu.outputFunction};
        u32_fast& frame {ppu.frame};
        u8_fast controller1 {0}, controller2 {0};

        Nes() {
            cpu.memory.readFunctions[0x4016] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address) {
                //TODO: open bus bits
                bool result = controller1 >> controller1Button & 0x01;
                controller1Button += 
                        !controllerStrobe 
                     && controller1Button < 8; 
                return result;
            };
            cpu.memory.writeFunctions[0x4016] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                if ((controllerStrobe = data & 0x01)) {
                    controller1Button = 0;
                    controller2Button = 0;
                }
            };
            cpu.memory.readFunctions[0x4017] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address) {
                //TODO: open bus bits
                bool result = controller2 >> controller2Button & 0x01;
                controller2Button += 
                        !controllerStrobe 
                     && controller2Button < 8; 
                return result;
            };

            cpu.timer.reload = 11;
            apu.timer.reload = 11;
            ppu.timer.reload = 3;
        }
        
        template <typename RomType, typename SramType>
        void load(RomType rom, SramType sram) {
            cart.load(rom, sram);
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
            cart.tick(ticks);
        }

        void writeMemory(
                const bool toPpu, 
                const u16 address, 
                const u8 data) {
            if (!toPpu || address <= 0x3FFF) {
                MappedMemory<>& memory = toPpu ? ppu.memory : cpu.memory;
                memory[address] = data;
            }
        }
        int readMemory(
                const bool fromPpu,
                const u16 address) {
            int value {-1};
            if (!fromPpu || address <= 0x3FFF) {
                MappedMemory<>& memory = fromPpu ? ppu.memory : cpu.memory;
                value = memory[address];
            }
            return value;
        }

        template <typename StateType>
        void dumpState(StateType& state) {
            cpu.dumpState(state);
            apu.dumpState(state);
            ppu.dumpState(state);

            std::vector<u8> cartState(cart.stateSize);
            cart.dumpState(cartState);
            state.write(cartState.data(), cart.stateSize); 
        }
        template <typename StateType>
        void loadState(StateType& state) {
            cpu.loadState(state);
            apu.loadState(state);
            ppu.loadState(state);

            std::vector<u8> cartState(cart.stateSize);
            state.read(cartState.data(), cart.stateSize);
            cart.loadState(cartState);
        }

        void ramdump(const char* const filename) {
            std::ofstream ramdumpFile {filename,
                    std::ofstream::binary | std::ofstream::trunc};
            auto ptr {ppu.memory.begin()};
            for (u32_fast i {0}; i <= 0x3FFF; ++i, ++ptr) {
                u8 byte {*ptr};
                ramdumpFile.write(reinterpret_cast<char*>(&byte), 1);
            }
            ptr = cpu.memory.begin();
            for (u32_fast i {0}; i <= 0xFFFF; ++i, ++ptr) {
                u8 byte {*ptr};
                ramdumpFile.write(reinterpret_cast<char*>(&byte), 1);
            }
            ramdumpFile.close();
        }
};
