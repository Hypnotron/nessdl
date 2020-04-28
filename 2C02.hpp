#include <array>
#include <vector>
#include <functional>
#include "byte.hpp"
#include "memory.hpp"

class Ppu {
    //TODO: Power-up state
    //TODO: Reset
    private:
        //Tick counter:
        u32_fast cycle {0};

        //TODO: Registers:
        u16_fast startAddress, address;
        u8_fast fineXScroll;
        u16_fast tileLow, tileHigh;
        bool paletteLow, paletteHigh;

        u8_fast tileIndexLatch;
        bool paletteLatchLow, paletteLatchHigh;
        u8_fast tileLatchLow;
        u8_fast tileLatchHigh;

        std::array<u8_fast, 256> primaryOam;
        std::array<u8_fast, 32> secondaryOam;
        std::array<u8_fast, 8> tileLows, tileHighs;
        std::array<u8_fast, 8> attributes;
        std::array<u8_fast, 8> xPositions;

        u16_fast nametableSelect;
        bool verticalPpuaddr;
        bool secondarySpritePatternTable;
        bool secondaryBackgroundPatternTable;
        bool eightBySixteenSprites;
        //TODO: master/slave
        bool nmiEnabled;

        bool grayscale;
        bool renderBackgroundFirstColumn;
        bool renderSpritesFirstColumn;
        bool renderBackground;
        bool renderSprites;
        bool emphasizeRed;
        bool emphasizeGreen;
        bool emphasizeBlue;

        bool firstWrite; 

        bool spriteOverflow;
        bool spriteZeroHit;
        bool inVblank;

        u8_fast oamaddr;
        u16_fast ppuaddrBuffer;

        //TODO: Per-operation temporary registers:
        u16_fast dot {0};
        std::vector<std::function<void()>>::const_iterator operation {
                operations[0].begin()};


        //TODO: Miscellaneous operations:
        std::function<void()> idle {[] () {}};
        //TODO: Operation table:
        //TODO: pixel output and shift before these operations
        std::vector<std::vector<std::function<void()>>> operations {
            /*0: Initialize visible scanlines */ {
                [&] () {
                    dot = 1;
                },
            },
            /*1: Visible scanlines */ {
                [&] () {
                    tileIndexLatch = memory[(address & 0x0FFF) | 0x2000];
                },
                idle,
                [&] () {
                    u8_fast tmp {memory[
                            (address & 0x0C00)
                          | 0x23C0
                          | ((address >> 4) & 0x38)
                          | ((address >> 2) & 0x07)]};
                    tmp >>= address & 0x02;
                    tmp >>= (address & 0x40) >> 4;
                    paletteLatchLow = tmp & 0x01;
                    paletteLatchHigh = (tmp >> 1) & 0x01;
                },
                idle,
                [&] () {
                    tileLatchLow = memory[
                            (secondaryBackgroundPatternTable << 12)
                          | (tileIndexLatch << 4)
                          | (address >> 12)]; 
                },
                idle,
                [&] () {
                    tileLatchHigh = memory[
                            (secondaryBackgroundPatternTable << 12)
                          | (tileIndexLatch << 4)
                          | 0x08
                          | (address >> 12)];
                },
                [&] () {
                    tileLow &= 0x00FF;
                    tileHigh &= 0x00FF;
                    tileLow |= tileLatchLow << 8;
                    tileHigh |= tileLatchHigh << 8;

                    paletteLow = paletteLatchLow;
                    paletteHigh = paletteLatchHigh;
                },
            },
        };
        //TODO: Rendering

    public:
        //TODO: Map memory (4KiB VRAM)
        MappedMemory<> memory{0};
};
