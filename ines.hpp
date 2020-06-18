#pragma once
#include <cassert>
#include <string>
#include <array>
#include <functional>
//TODO: remove debug module
#include "debug.hpp"
#include "byte.hpp"
#include "memory.hpp"
#include "2A03.hpp"

namespace ines {
    template <typename RomType>
    bool isValid(RomType& rom) {
        std::array<u8, 4> magic;
        rom.read(reinterpret_cast<char*>(magic.begin()), 4);
        rom.seekg(-4, std::ios::cur);
        return 
                readBytes<4, u32, Endianness::BIG>(magic.begin())
             == 0x4E45531A; 
        //        N.E.S... 
    }

    template <typename RomType, typename SramType>
    void load(
            RomType rom,
            SramType sram,
            MappedMemory<>& cpuMemory,
            MappedMemory<>& ppuMemory,
            std::function<void(const u8_fast)>& cartTick) {
        enum Mirroring : u8_fast {
            HORIZONTAL,
            VERTICAL,
            FOUR_SCREEN,
        };

        std::array<u8, 0x10> header;
        rom.read(reinterpret_cast<char*>(header.begin()), 0x10);

        u8_fast prgSize {header[4]};
        u8_fast chrSize {header[5]};
        bool chrRam {chrSize == 0};

        bool saveRam = header[6] & 0x02;
        bool trainer = header[6] & 0x04;
        Mirroring mirroring {
                header[6] & 0x08 ? FOUR_SCREEN :
                header[6] & 0x01 ? VERTICAL : HORIZONTAL}; 
        u8_fast mapperNumber = (header[7] & 0xF0) | (header[6] >> 4);

        //Miscellaneous mapped memory lambdas:
        const auto openBusRead = [] (
                MappedMemory<>* const memory,
                const u16 address) {
            //TODO: Proper open bus read
            //debug::log << "OPEN BUS READ AT " << address << "\n";
            return static_cast<u8>(0);
        };
        const auto openBusWrite = [] (
                MappedMemory<>* const memory,
                const u16 address,
                const u8 data) {
            //TODO: Proper open bus write
            //debug::log << "OPEN BUS WRITE OF " << static_cast<u16>(data)
            //           << " AT " << address << "\n";
        };
        const auto standardRead = [] (
                MappedMemory<>* const memory,
                const u16 address) {
            return memory->memory[address];
        };
        const auto standardWrite = [] (
                MappedMemory<>* const memory,
                const u16 address,
                const u8 data) {
            memory->memory[address] = data;
        };

        switch (mapperNumber) {
        case 0: {
            cpuMemory.readFunctions[0x5FFF] = openBusRead; 
            cpuMemory.writeFunctions[0x5FFF] = openBusWrite;

            //TODO: Proper SRAM handling
            if (saveRam) {
                cpuMemory.readFunctions[0x7FFF] = standardRead;
                cpuMemory.writeFunctions[0x7FFF] = standardWrite;
            }
            else {
                cpuMemory.readFunctions[0x7FFF] = openBusRead;
                cpuMemory.writeFunctions[0x7FFF] = openBusWrite;
            }
            
            cpuMemory.resize(prgSize == 1 ? 0xC000 : 0x10000);
            rom.read(reinterpret_cast<char*>(
                    cpuMemory.memory.data() + 0x8000), 
                    prgSize == 1 ? 0x4000 : 0x8000); 
            cpuMemory.readFunctions[0xFFFF] = [=] ( 
                    MappedMemory<>* const memory,
                    const u16 address) {
                return memory->memory[address 
                        & (prgSize == 1 ? 0xBFFF : 0xFFFF)];
            };
            cpuMemory.writeFunctions[0xFFFF] = openBusWrite; 

            ppuMemory.resize(0x4000);
            rom.read(reinterpret_cast<char*>(
                    ppuMemory.memory.data() + 0x2000),
                    0x2000);
            ppuMemory.readFunctions[0x1FFF] = [] (
                    MappedMemory<>* const memory,
                    const u16 address) {
                return memory->memory[address + 0x2000];
            };
            ppuMemory.writeFunctions[0x1FFF] = openBusWrite;

            u16 mirroringBitmask;
            switch (mirroring) {
            case FOUR_SCREEN:
                mirroringBitmask = 0x0FFF;
            break;
            case HORIZONTAL:
                mirroringBitmask = 0x0BFF;
            break;
            case VERTICAL:
                mirroringBitmask = 0x07FF;
            break;
            }
            ppuMemory.readFunctions[0x3EFF] = [=] (
                    MappedMemory<>* const memory,
                    const u16 address) {
                return memory->memory[address & mirroringBitmask];
            }; 
            ppuMemory.writeFunctions[0x3EFF] = [=] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                memory->memory[address & mirroringBitmask] = data;
            };

            cartTick = [] (const u8_fast) {};
        break; }

        case 1: {
            //TODO: variant support
            static u32_fast lastControlWriteCycle; lastControlWriteCycle = 0;
            static u32_fast lastSramWriteCycle; lastSramWriteCycle = 0;
            static u32_fast cycle; cycle = 24;
            static u8 shiftRegister {0};
            static u8_fast shiftCount; shiftCount = 0;
            static u8_fast mmcMirroring {0};
            static u8_fast prgMode; prgMode = 3;
            static bool contiguousChr {false};
            static u8_fast chrBank0; chrBank0 = 0;
            static u8_fast chrBank1; chrBank1 = 0;
            static u8_fast prgRomBank; prgRomBank = 0;
            static u8_fast prgRamBank; prgRamBank = 0;
            static bool prgRamEnable {false}; 
            chrSize = chrRam ? 1 : chrSize; 

            cpuMemory.readFunctions[0x5FFF] = openBusRead;
            cpuMemory.writeFunctions[0x5FFF] = openBusWrite;

            cpuMemory.readFunctions[0x7FFF] = [&, prgSize]  (
                    MappedMemory<>* const memory,
                    const u16 address) {
                if (prgRamEnable) {
                    return memory->memory[
                            address 
                          + 0x2000
                          + prgSize * 0x4000
                          + prgRamBank * 0x2000];
                }
                else {
                    return openBusRead(memory, address);
                }
            };
            cpuMemory.writeFunctions[0x7FFF] = [&, prgSize] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                if (prgRamEnable) {
                    lastSramWriteCycle = cycle;
                    memory->memory[
                            address
                          + 0x2000
                          + prgSize * 0x4000
                          + prgRamBank * 0x2000] = data;
                }
                else {
                    openBusWrite(memory, address, data);
                }
            };

            cpuMemory.resize(prgSize * 0x4000 + 0x10000);
            rom.read(reinterpret_cast<char*>(
                    cpuMemory.memory.data() + 0x8000),
                    prgSize * 0x4000);
            if (saveRam) {
                sram.read(reinterpret_cast<char*>(
                        cpuMemory.memory.data() + 0x8000 + prgSize * 0x4000),
                        0x8000);
                sram.seekg(-0x8000, std::ios::cur);
            }
            cpuMemory.readFunctions[0xFFFF] = [&, prgSize] (
                    MappedMemory<>* const memory,
                    const u16 address) {
                switch (prgMode) {
                case 0:
                case 1:
                    return memory->memory[
                            address 
                          + (prgRomBank & 0x1E) * 0x4000];
                break;
                case 2:
                    return address <= 0xBFFF 
                          ? memory->memory[
                                    address
                                  + (prgRomBank & 0x10) * 0x4000]
                          : memory->memory[
                                    address 
                                  + (prgRomBank - 1) * 0x4000];
                break;
                default:
                    return address <= 0xBFFF
                          ? memory->memory[
                                    address 
                                  + prgRomBank * 0x4000]
                          : memory->memory[
                                    address 
                                  + ((prgSize > 16 && !(prgRomBank & 0x10))
                                  ? 14
                                  : prgSize - 2) * 0x4000];
                }
            };
            cpuMemory.writeFunctions[0xFFFF] = [&, prgSize, chrSize] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                if (static_cast<u32_fast>(
                        cycle - lastControlWriteCycle) >= 12 * 2) {
                    lastControlWriteCycle = cycle;
                    shiftRegister >>= 1;
                    setBit(shiftRegister, 4, data & 0x01);
                    if (data & 0x80) {
                        shiftRegister |= 0x0C; 
                        shiftCount = 4;
                    }
                    if (++shiftCount == 5) {
                        shiftCount = 0;

                        if (        
                                address >= 0x8000 && address <= 0x9FFF 
                             || (data & 0x80)) {
                            mmcMirroring = shiftRegister & 0x03;
                            prgMode = shiftRegister >> 2 & 0x03;
                            contiguousChr = !(shiftRegister & 0x10);
                        }
                        else if (address >= 0xA000 && address <= 0xBFFF) {
                            chrBank0 = shiftRegister;
                            setBit(prgRamBank, 1, shiftRegister & 0x04);
                            setBit(prgRamBank, 0, shiftRegister & 0x08);
                            setBit(prgRomBank, 4, shiftRegister & 0x10);
                        }
                        else if (address >= 0xC000 && address <= 0xDFFF) {
                            chrBank1 = shiftRegister; 
                            setBit(prgRamBank, 1, shiftRegister & 0x04);
                            setBit(prgRamBank, 0, shiftRegister & 0x08);
                            setBit(prgRomBank, 4, shiftRegister & 0x10);
                        }
                        else if (address >= 0xE000 && address <= 0xFFFF) {
                            prgRomBank &= 0x10;
                            prgRomBank |= shiftRegister & 0x0F;
                            prgRamEnable = !(shiftRegister & 0x10);
                        }

                        chrBank0 %= chrSize * 2; 
                        chrBank1 %= chrSize * 2; 
                        prgRomBank %= prgSize; 
                    }
                }
            };

            ppuMemory.resize((chrSize + 1) * 0x2000);
            rom.read(reinterpret_cast<char*>(
                    ppuMemory.memory.data() + 0x2000),
                    chrSize * 0x2000);
            ppuMemory.readFunctions[0x1FFF] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address) {
                if (contiguousChr) {
                    return memory->memory[
                            address 
                         + (chrBank0 & 0xFE) * 0x1000
                         + 0x2000];
                }
                else {
                    return address <= 0x0FFF
                          ? memory->memory[
                                    address 
                                  + chrBank0 * 0x1000
                                  + 0x2000]
                          : memory->memory[
                                    address 
                                  + chrBank1 * 0x1000
                                  + 0x1000];
                }
            };
            if (chrRam) {
                ppuMemory.writeFunctions[0x1FFF] = [&] (
                        MappedMemory<>* const memory,
                        const u16 address,
                        const u8 data) {
                    if (contiguousChr) {
                        memory->memory[
                                address
                              + (chrBank0 & 0xFE) * 0x1000
                              + 0x2000] = data;
                    }
                    else {
                        if (address <= 0x0FFF) {
                            memory->memory[
                                    address 
                                  + chrBank0 * 0x1000
                                  + 0x2000] = data;
                        }
                        else {
                            memory->memory[
                                    address 
                                  + chrBank1 * 0x1000
                                  + 0x1000] = data;
                        }
                    }
                };
            }
            else {
                ppuMemory.writeFunctions[0x1FFF] = openBusWrite;
            }

            ppuMemory.readFunctions[0x3EFF] = [&, chrSize] (
                    MappedMemory<>* const memory,
                    const u16 address) {
                switch (mmcMirroring) {
                case 0:
                    return memory->memory[address & 0x03FF];
                break;
                case 1:
                    return memory->memory[(address & 0x03FF) + 0x0400];
                break;
                case 2:
                    return memory->memory[address & 0x07FF];
                break;
                default:
                    return memory->memory[
                            (address & 0x0BFF)
                          - (address & 0x0800 ? 0x0400 : 0x0000)];
                }
            };
            ppuMemory.writeFunctions[0x3EFF] = [&, chrSize] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                switch (mmcMirroring) {
                case 0:
                    memory->memory[address & 0x03FF] = data;
                break;
                case 1:
                    memory->memory[(address & 0x03FF) + 0x0400] = data;
                break;
                case 2:
                    memory->memory[address & 0x07FF] = data;
                break;
                case 3:
                    memory->memory[
                            (address & 0x0BFF)
                          - (address & 0x0800 ? 0x0400 : 0x0000)] = data;
                break;
                }
            };

            cartTick = [&, saveRam, prgSize, sram] 
                    (const u8_fast ticks) mutable {
                cycle += ticks;
                if (saveRam && static_cast<u32_fast>(
                        cycle - lastSramWriteCycle) == 21441960) {
                    sram.write(reinterpret_cast<const char*>(
                            cpuMemory.memory.data()
                          + 0x8000
                          + prgSize * 0x4000), 0x8000);
                    sram.seekp(-0x8000, std::ios::cur);
                }
            };
        break; }
        case 3: {
            static u8_fast chrBank {0};

            cpuMemory.readFunctions[0x7FFF] = openBusRead; 
            cpuMemory.writeFunctions[0x7FFF] = openBusWrite;

            cpuMemory.resize(prgSize * 0x4000 + 0x8000);
            rom.read(reinterpret_cast<char*>(
                    cpuMemory.memory.data() + 0x8000), 
                    prgSize * 0x4000); 
            cpuMemory.readFunctions[0xFFFF] = [=] ( 
                    MappedMemory<>* const memory,
                    const u16 address) {
                return memory->memory[address 
                        & (prgSize == 1 ? 0xBFFF : 0xFFFF)];
            };
            cpuMemory.writeFunctions[0xFFFF] = [&, chrSize] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                chrBank = data % chrSize;
            };

            ppuMemory.resize((chrSize + 1) * 0x2000);
            rom.read(reinterpret_cast<char*>(
                    ppuMemory.memory.data() + 0x2000),
                    chrSize * 0x2000);
            ppuMemory.readFunctions[0x1FFF] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address) {
                return memory->memory[
                        address 
                      + (chrBank + 1) * 0x2000];
            };
            ppuMemory.writeFunctions[0x1FFF] = openBusWrite;

            u16 mirroringBitmask;
            switch (mirroring) {
            case FOUR_SCREEN:
                mirroringBitmask = 0x0FFF;
            break;
            case HORIZONTAL:
                mirroringBitmask = 0x0BFF;
            break;
            case VERTICAL:
                mirroringBitmask = 0x07FF;
            break;
            }
            ppuMemory.readFunctions[0x3EFF] = [=] (
                    MappedMemory<>* const memory,
                    const u16 address) {
                return memory->memory[address & mirroringBitmask];
            }; 
            ppuMemory.writeFunctions[0x3EFF] = [=] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                memory->memory[address & mirroringBitmask] = data;
            };

            cartTick = [] (const u8_fast) {};
        break; }

        default:
            assert(false && mapperNumber && "Mapper unavailable"); 

        }
    }
}
        
