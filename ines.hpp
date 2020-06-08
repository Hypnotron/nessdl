#pragma once
#include <cassert>
#include <string>
#include <array>
#include <functional>
#include "debug.hpp"
#include "byte.hpp"
#include "memory.hpp"
#include "2A03.hpp"

namespace ines {
    template <typename RomType>
    bool isValid(RomType& rom) {
        std::array<u8, 4> magic;
        rom.read(reinterpret_cast<char*>(magic.begin()), 4);
        return 
                readBytes<4, u32, Endianness::BIG>(magic.begin())
             == 0x4E45531A; 
        //        N.E.S... 
    }

    template <typename RomType>
    void load(
            RomType& rom,
            MappedMemory<>& cpuMemory,
            MappedMemory<>& ppuMemory) {
        enum Mirroring : u8_fast {
            HORIZONTAL,
            VERTICAL,
            FOUR_SCREEN,
        };

        rom.seekg(0, std::ios::beg);

        std::array<u8, 0x10> header;
        rom.read(reinterpret_cast<char*>(header.begin()), 0x10);

        u8_fast prgSize {header[4]};
        u8_fast chrSize {header[5]};

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
            return 0;
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
        case 0:
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
            rom.read(reinterpret_cast<char*>(ppuMemory.memory.data()), 0x2000);
            ppuMemory.readFunctions[0x1FFF] = standardRead;
            ppuMemory.writeFunctions[0x1FFF] = openBusWrite;


            u16 mirroringBitmask;
            switch (mirroring) {
            case FOUR_SCREEN:
                mirroringBitmask = 0x2FFF;
            break;
            case HORIZONTAL:
                mirroringBitmask = 0x2BFF;
            break;
            case VERTICAL:
                mirroringBitmask = 0x27FF;
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
        break;

        default:
            assert(false && mapperNumber && "Mapper unavailable"); 
        }
    }
}
        
