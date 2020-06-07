#include <array>
#include <vector>
#include <functional>
#include "byte.hpp"
//TODO: remove
#include "debug.hpp"
#include "memory.hpp"
#include "2A03.hpp"

class Ppu {
    private:
        //Cpu:
        Cpu& cpu;

        //Registers:
        u8_fast dataLatch;

        u16_fast startAddress {0}, address {0};
        u8_fast fineXScroll;
        u16_fast tileLow, tileHigh;
        u16_fast paletteLow, paletteHigh;

        u8_fast tileIndexLatch;
        u8_fast paletteLatchLow, paletteLatchHigh;
        u8_fast tileLatchLow;
        u8_fast tileLatchHigh;

        std::array<u8_fast, 256> primaryOam {};
        std::array<u8_fast, 32> secondaryOam {};
        std::array<u8_fast, 8> tileLows, tileHighs {};
        std::array<u8_fast, 8> attributes {};
        std::array<u8, 8> xPositions {};
        //TODO: Handle empty sprites in secondary OAM

        bool verticalPpuaddr;
        bool secondarySpritePatternTable;
        bool secondaryBackgroundPatternTable;
        //TODO: 8x16 sprites
        bool eightBySixteenSprites;
        //TODO: master/slave
        bool nmiEnabled; 

        u8_fast grayscaleMask {0x3F};
        bool renderBackgroundFirstColumn {false};
        bool renderSpritesFirstColumn {false};
        bool renderBackground {false};
        bool renderSprites {false};
        //TODO: emphasis
        bool emphasizeRed {false};
        bool emphasizeGreen {false};
        bool emphasizeBlue {false};

        bool firstWrite {true}; 

        bool spriteOverflow;
        bool spriteZeroHit {false};
        bool inVblank;

        u8      oamaddr {0};
        u8_fast oamdata;
        u8_fast ppudata {0};

        //Palette:
        const std::array<u8_fast, 192> palette {
                0x5c, 0x5c, 0x5c, 0x00, 0x22, 0x67, 0x13, 0x12, 0x80, 
                0x2e, 0x06, 0x7e, 0x46, 0x00, 0x60, 0x53, 0x02, 0x31, 
                0x51, 0x0a, 0x02, 0x41, 0x19, 0x00, 0x28, 0x29, 0x00, 
                0x0d, 0x37, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x3c, 0x0a, 
                0x00, 0x31, 0x3b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                0x00, 0x00, 0x00, 0xa7, 0xa7, 0xa7, 0x1e, 0x55, 0xb7, 
                0x3f, 0x3d, 0xda, 0x66, 0x2b, 0xd6, 0x88, 0x22, 0xac, 
                0x9a, 0x24, 0x6b, 0x98, 0x32, 0x25, 0x81, 0x47, 0x00, 
                0x5d, 0x5f, 0x00, 0x36, 0x73, 0x00, 0x18, 0x7d, 0x00,
                0x09, 0x7a, 0x32, 0x0b, 0x6b, 0x79, 0x00, 0x00, 0x00, 
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0xff, 
                0x6a, 0xa7, 0xff, 0x8f, 0x8d, 0xff, 0xb9, 0x79, 0xff, 
                0xdd, 0x6f, 0xff, 0xf1, 0x72, 0xbe, 0xee, 0x81, 0x73, 
                0xd6, 0x98, 0x37, 0xb0, 0xb2, 0x18, 0x86, 0xc7, 0x1c, 
                0x64, 0xd1, 0x41, 0x52, 0xce, 0x81, 0x54, 0xbe, 0xcd, 
                0x45, 0x45, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                0xfe, 0xff, 0xff, 0xc0, 0xda, 0xff, 0xd0, 0xcf, 0xff, 
                0xe2, 0xc6, 0xff, 0xf1, 0xc2, 0xff, 0xf9, 0xc3, 0xe4, 
                0xf8, 0xca, 0xc4, 0xee, 0xd4, 0xa9, 0xde, 0xdf, 0x9b, 
                0xcc, 0xe7, 0x9d, 0xbd, 0xec, 0xae, 0xb5, 0xea, 0xca, 
                0xb6, 0xe4, 0xea, 0xb0, 0xb0, 0xb0, 0x00, 0x00, 0x00, 
                0x00, 0x00, 0x00,
        };
       
        //Miscellaneous operations:
        std::function<void()> idle {[] () {
            //TODO: remove all debug::log
            //debug::log << "IDLE" << std::endl;
        }};
        void incrementX() {
            if ((address++ & 0x001F) == 0x001F) {
                --address &= 0xFFE0;
                address ^= 0x0400;
            }
        }
        //Adapted from Nesdev's algorithm:
        void incrementY() {
            if ((address += 0x1000) & 0x8000) {
                address -= 0x8000; 
                u8_fast coarseY = (address & 0x03E0) >> 5;
                if (coarseY == 29) {
                    address ^= 0x0800;
                }
                coarseY = coarseY == 29 || coarseY == 31 
                      ? 0
                      : coarseY + 1;
                address &= 0xFC1F;
                address |= coarseY << 5;
            }
        }

        //Operation tables:
        std::vector<std::vector<std::function<void()>>> operations {
            /*0: Initialize visible scanlines */ {
                [&] () {
                    //debug::log << "0.0" << std::endl;
                    operation = operations[1].begin();
                    operationStep = 0;
                },
            },
            /*1: Background memory accesses */ {
                [&] () {
                    //debug::log << "1.0" << std::endl;
                    tileIndexLatch = renderBackground
                          ? memory[(address & 0x0FFF) | 0x2000]
                          : 0;

                    if (scanline == -1 && dot == 1) {
                        spriteOverflow = false;
                        spriteZeroHit = false;
                        inVblank = false;
                    }
                },
                idle,
                [&] () {
                    //debug::log << "1.2" << std::endl;
                    u8_fast tmp = renderBackground
                          ? memory[
                            (address & 0x0C00)
                          | 0x23C0
                          | ((address >> 4) & 0x38)
                          | ((address >> 2) & 0x07)]
                          : 0;
                    tmp >>= address & 0x02 ? 2 : 0;
                    tmp >>= address & 0x40 ? 4 : 0; 
                    paletteLatchLow = tmp & 0x01 ? 0xFF : 0x00;
                    paletteLatchHigh = (tmp >> 1) & 0x01 ? 0xFF : 0x00;
                },
                idle,
                [&] () {
                    //debug::log << "1.4" << std::endl;
                    tileLatchLow = renderBackground || renderSprites
                          ? bitwiseReverse<1, u8>(memory[
                            (secondaryBackgroundPatternTable << 12)
                          | (tileIndexLatch << 4)
                          | (address >> 12)])
                          : 0;
                },
                idle,
                [&] () {
                    //debug::log << "1.6" << std::endl;
                    tileLatchHigh = renderBackground || renderSprites
                          ? bitwiseReverse<1, u8>(memory[
                            (secondaryBackgroundPatternTable << 12)
                          | (tileIndexLatch << 4)
                          | 0x08
                          | (address >> 12)])
                          : 0;
                },
                [&] () {
                    //debug::log << "1.7" << std::endl;
                    tileLow &= 0x00FF;
                    tileHigh &= 0x00FF;
                    tileLow |= tileLatchLow << 8;
                    tileHigh |= tileLatchHigh << 8;

                    paletteLow &= 0x00FF; 
                    paletteHigh &= 0x00FF;
                    paletteLow |= paletteLatchLow << 8;
                    paletteHigh |= paletteLatchHigh << 8;

                    if (renderBackground || renderSprites) {
                        incrementX();
                        if (dot == 256) {
                            incrementY();
                        }
                    }

                    operation = operations[
                        dot == 256
                      ? 2
                      : dot == 336 
                      ? 3 
                      : 1].begin();
                    operationStep = 0;
                },
            },
            /*2: Sprite memory accesses */ {
                [&] () {
                    if (renderBackground || renderSprites) {
                        static_cast<u8>(memory[(address & 0x00FF) | 0x2000]);
                        if (dot == 257) {
                            setBit(address, 10, startAddress & 0x0400);
                            address &= 0xFFE0;
                            address |= startAddress & 0x001F;
                        }
                    }
                    //debug::log << "2.0" << std::endl;
                },
                idle,
                [&] () {
                    //debug::log << "2.2" << std::endl;
                    if (renderBackground || renderSprites) {
                        static_cast<u8>(memory[
                                (address & 0x0C00)
                              | 0x23C0
                              | ((address >> 4) & 0x38)
                              | ((address >> 2) & 0x07)]);
                    }
                    attributes[(dot - 259) / 8] = 
                            secondaryOam[(dot - 259) / 2 + 2];
                },
                [&] () {
                    //debug::log << "2.3" << std::endl;
                    xPositions[(dot - 260) / 8] =
                            secondaryOam[(dot - 260) / 2 + 3];
                },
                [&] () {
                    //debug::log << "2.4" << std::endl;
                    u8_fast spriteHeight = eightBySixteenSprites ? 16 : 8;
                    u8_fast yOffset =
                            attributes[(dot - 261) / 8] & 0x80

                          ? secondaryOam[(dot - 261) / 2] 
                          + spriteHeight 
                          - scanline

                          : scanline
                          - secondaryOam[(dot - 261) / 2]
                          - 1;

                    tileLows[(dot - 261) / 8] = 
                            renderBackground || renderSprites
                          ? memory[
                            (secondarySpritePatternTable << 12)
                          | (secondaryOam[(dot - 261) / 2 + 1] << 4)
                          | yOffset] 
                          : 0;

                    if (!(attributes[(dot - 261) / 8] & 0x40)) {
                        tileLows[(dot - 261) / 8] = bitwiseReverse<1, u8>(
                                tileLows[(dot - 261) / 8]);
                    }
                },
                idle,
                [&] () {
                    //debug::log << "2.6" << std::endl;
                    u8_fast spriteHeight = eightBySixteenSprites ? 16 : 8;
                    u8_fast yOffset =
                            attributes[(dot - 263) / 8] & 0x80

                          ? secondaryOam[(dot - 263) / 2] 
                          + spriteHeight 
                          - scanline

                          : scanline
                          - secondaryOam[(dot - 263) / 2]
                          - 1;

                    tileHighs[(dot - 263) / 8] = 
                            renderBackground || renderSprites
                          ? memory[
                            (secondarySpritePatternTable << 12)
                          | (secondaryOam[(dot - 263) / 2 + 1] << 4)
                          | 0x08
                          | yOffset] 
                          : 0;

                    if (!(attributes[(dot - 263) / 8] & 0x40)) {
                        tileHighs[(dot - 263) / 8] = bitwiseReverse<1, u8>(
                                tileHighs[(dot - 263) / 8]);
                    }
                },
                [&] () {
                    //debug::log << "2.7" << std::endl;
                    operation = operations[dot < 320 ? 2 : 1].begin();
                    operationStep = 0;
                },
            },
            /*3: Dummy nametable accesses */ {
                [&] () {
                    //debug::log << "3.0" << std::endl;
                    if (renderBackground || renderSprites) {
                        static_cast<u8>(memory[(address & 0x00FF) | 0x2000]);
                    }
                },
                idle,
                [&] () {
                    //debug::log << "3.2" << std::endl;
                    if (renderBackground || renderSprites) {
                        static_cast<u8>(memory[(address & 0x00FF) | 0x2000]);
                    }
                },
                [&] () {
                    //debug::log << "3.3" << std::endl;
                    operation = operations[scanline < 240 ? 0 : 4].begin();
                    operationStep = 0;
                },
            },
            /*4: Vertical blanking */ {
                [&] () {
                    //debug::log << "4.0" << std::endl;
                    //TODO: PPUSTATUS read race condititon
                    if (scanline == 241 && dot == 1) {
                        inVblank = true;
                        if (nmiEnabled) {
                            cpu.edgeNmi();
                        }
                    }
                    operationStep = 0;
                },
            },
        };
        //Sprite evaluation:
        std::vector<std::vector<std::function<void()>>> spriteEvalOps {
            /*0: Idling */ { 
                [&] () {
                    if (dot == 0 && scanline >= 0 && scanline <= 239) {
                        spriteEvalOp = spriteEvalOps[1].begin();
                    }
                    spriteEvalOpStep = 0;
                }
            },
            /*1: Clear secondary OAM */ {
                [&] () {
                    oamdata = 0xFF;
                    n = m = spritesEvaluated = 0;
                },
                [&] () {
                    secondaryOam[dot / 2 - 1] = oamdata;
                    spriteEvalOp = spriteEvalOps[dot >= 64 ? 2 : 1].begin();
                    spriteEvalOpStep = 0;
                },
            },
            /*2: Sprite evaluation */ {
                [&] () {
                    oamdata = primaryOam[n * 4 + m]; 
                },
                [&] () {
                    if (spritesEvaluated < 8) { 
                        secondaryOam[spritesEvaluated * 4] = oamdata;
                    }

                    if (
                            scanline < oamdata || scanline - oamdata 
                            >= (eightBySixteenSprites ? 16 : 8)) {
                        m += spritesEvaluated >= 8;
                        m &= 0x03;
                        ++n;

                        spriteEvalOp = spriteEvalOps[n == 64 ? 0 : 2].begin();
                        spriteEvalOpStep = 0;
                    }
                    else if (spritesEvaluated >= 8) {
                        spriteOverflow = true;

                        spriteEvalOp = spriteEvalOps[0].begin();
                        spriteEvalOpStep = 0;
                    }
                },
                [&] () {
                    oamdata = primaryOam[n * 4 + 1];
                },
                [&] () {
                    secondaryOam[spritesEvaluated * 4 + 1] = oamdata;
                },
                [&] () {
                    oamdata = primaryOam[n * 4 + 2] & 0xE3;
                },
                [&] () {
                    secondaryOam[spritesEvaluated * 4 + 2] = 
                            oamdata | ((n == 0) << 2); 
                },
                [&] () {
                    oamdata = primaryOam[n * 4 + 3];
                },
                [&] () {
                    secondaryOam[spritesEvaluated * 4 + 3] = oamdata;
                    ++spritesEvaluated;
                    ++n;

                    spriteEvalOp = spriteEvalOps[n == 64 ? 0 : 2].begin();
                    spriteEvalOpStep = 0;
                },
            },
        };

        //Per-operation temporary registers:
        u16_fast dot {0};
        s16_fast scanline {0};
        u8_fast n, m, spritesEvaluated;
        std::vector<std::function<void()>>::const_iterator operation { 
                operations[0].begin()};
        u8_fast operationStep {1};
        std::vector<std::function<void()>>::const_iterator spriteEvalOp {
                spriteEvalOps[0].begin()};
        u8_fast spriteEvalOpStep {1};
 

    public:
        //Tick counter:
        u32_fast cycle {0};

        //Frame counter (no, not the APU):
        u32_fast frame {0};

        std::function<void(
                const u8_fast x, 
                const u8_fast y, 
                const u32 pixel)> outputFunction {[] (
                const u8_fast, 
                const u8_fast, 
                const u32) {}};

        //Memory:
        MappedMemory<> memory{0};

        Counter<s8_fast> timer{0, [&] () {
            //Pixel output:
            if (dot >= 1 && dot <= 256 && scanline >= 0 && scanline <= 239) {
                u8_fast bgValue =
                        ((tileHigh >> fineXScroll & 1) << 1)
                      | (tileLow >> fineXScroll & 1);
                u8_fast bgPixel {
                        renderBackground 
                     && (dot > 8 || renderBackgroundFirstColumn)
                     && bgValue 
                      ? memory[
                                0x3F00
                              | ((paletteHigh >> fineXScroll & 1) << 3)
                              | ((paletteLow >> fineXScroll & 1) << 2)
                              | bgValue]
                      : address >= 0x3F00 && address <= 0x3FFF
                      ? memory[address]
                      : memory[0x3F00]};
                bgPixel &= grayscaleMask;

                u8_fast spriteValue {0};
                bool spriteZero {false};
                bool foregroundPriority {false};
                u8_fast spritePalette {0};
                if (
                        renderSprites 
                     && (dot > 8 || renderSpritesFirstColumn)) {
                    for (u8_fast i {0}; i < 8 && spriteValue == 0; ++i) {
                        if (xPositions[i] == 0) {
                            spriteValue = 
                                    (tileHighs[i] & 1) << 1
                                  | (tileLows[i] & 1);
                            spriteZero = attributes[i] & 0x04;
                            foregroundPriority = !(attributes[i] & 0x20);
                            spritePalette = attributes[i] & 0x03;
                        }
                    }
                }
                u8_fast spritePixel {memory[
                        0x3F10
                      + (spritePalette << 2)
                      + spriteValue]};
                spritePixel &= grayscaleMask;

                u8_fast pixel {
                        spriteValue && !bgValue
                     || spriteValue && bgValue && foregroundPriority
                      ? spritePixel
                      : bgPixel};

                spriteZeroHit |= spriteZero && spriteValue && bgValue;

                outputFunction(dot - 1, scanline, 
                        palette[pixel * 3    ] << 16 
                      | palette[pixel * 3 + 1] << 8
                      | palette[pixel * 3 + 2]);
                
                //Shift register updates:
                tileLow >>= 1;
                tileHigh >>= 1;
                paletteLow >>= 1;
                paletteHigh >>= 1;
                for (u8_fast i {0}; i < 8; ++i) {
                    //DISCREPANCY: don't know whether to shift immediately 
                    //after x becomes 0 or wait another cycle, waiting:
                    if (--(xPositions[i]) == 255) {
                        xPositions[i] = 0;
                        tileLows[i] >>= 1;
                        tileHighs[i] >>= 1;
                    }
                }
            }
            
            //TODO: remove
            //debug::log << "DOT: " << std::dec << dot << "\n";
            //debug::log << "SCANLINE: " << scanline << "\n";
            //debug::log << "ADDRESS: " << std::hex << address << "\n";

            //Main render operation:
            (*operation)();
            operation += operationStep;
            operationStep = 1;

            //Sprite evaluation operation:
            if (renderBackground || renderSprites) {
                (*spriteEvalOp)();
                spriteEvalOp += spriteEvalOpStep;
                spriteEvalOpStep = 1;
            }

            //Special scroll adjustments:
            if (scanline == -1 && dot >= 280 && dot <= 304) {
                address &= 0x041F;
                address |= startAddress & 0x7800;
                address |= startAddress & 0x03E0;
            }

            //Dot and scanline increments:
            ++dot;
            if (
                    (renderBackground || renderSprites)
                 && scanline == -1
                 && dot == 340
                 && frame % 2) {
                (*operation)();
                operation += operationStep;
                operationStep = 1;
                ++dot;
            }
            if (dot == 341) {
                dot = 0;
                if (++scanline == 261) {
                    ++frame;
                    scanline = -1;
                    operation = operations[0].begin(); 
                }
            }
        }};

        void tick(const u8_fast ticks = 1) {
            timer.tick(ticks);
        }

        void reset() {
            memory[0x2000] = 0x00;
            memory[0x2001] = 0x00;
            memory[0x2005] = 0x00;

            firstWrite = true;
            ppudata = 0x00;

            operation = operations[0].begin();
            operationStep = 1;
            spriteEvalOp = spriteEvalOps[0].begin();
            spriteEvalOpStep = 1;
            dot = 0;
            scanline = 0;

            frame = 0;
            cycle = 0;
        }

        Ppu(Cpu& cpu) 
              : cpu{cpu} { 
            cpu.memory.writeFunctions[0x3FFF] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                (*memory)[address & 0x2007] = data;
            };
            cpu.memory.readFunctions[0x3FFF] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address) {
                return (*memory)[address & 0x2007];
            };
            cpu.memory.writeFunctions[0x2000] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                dataLatch = data;
                setBit(startAddress, 10, data & 0x01); 
                setBit(startAddress, 11, data & 0x02);
                verticalPpuaddr = data & 0x04;
                secondarySpritePatternTable = data & 0x08;
                secondaryBackgroundPatternTable = data & 0x10;
                eightBySixteenSprites = data & 0x20;
                //TODO: master/slave
                if (!nmiEnabled && data & 0x80 && inVblank) {
                    cpu.edgeNmi();
                }
                nmiEnabled = data & 0x80;
            };
            cpu.memory.readFunctions[0x2000] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address) {
                return dataLatch;
            };
            cpu.memory.writeFunctions[0x2001] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                dataLatch = data;
                grayscaleMask = data & 0x01 ? 0x30 : 0x3F;
                renderBackgroundFirstColumn = data & 0x02;
                renderSpritesFirstColumn = data & 0x04;
                renderBackground = data & 0x08;
                renderSprites = data & 0x10;
                emphasizeRed = data & 0x20;
                emphasizeGreen = data & 0x40;
                emphasizeBlue = data & 0x80;
            };
            cpu.memory.readFunctions[0x2001] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address) {
                return dataLatch;
            };
            cpu.memory.writeFunctions[0x2002] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                dataLatch = data;
            };
            cpu.memory.readFunctions[0x2002] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address) {
                u8 result = dataLatch & 0x1F;
                result |= spriteOverflow << 5;
                result |= spriteZeroHit << 6;
                result |= inVblank << 7;
                inVblank = false;
                firstWrite = true;
                return result;
            };
            cpu.memory.writeFunctions[0x2003] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                oamaddr = dataLatch = data;
            };
            cpu.memory.readFunctions[0x2003] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address) {
                return dataLatch;
            };
            cpu.memory.writeFunctions[0x2004] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                if (scanline >= 240 || (!renderBackground && !renderSprites)) {
                    primaryOam[oamaddr] = dataLatch = data; 
                    ++oamaddr;
                }
            };
            cpu.memory.readFunctions[0x2004] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address) {
                if (scanline >= 240 || (!renderBackground && !renderSprites)) {
                    return primaryOam[oamaddr];
                }
                else {
                    return oamdata;
                }
            };
            cpu.memory.writeFunctions[0x2005] = [&] (
                    MappedMemory<>* const memory,
                    const u16,
                    const u8 data) {
                dataLatch = data;

                if (firstWrite) {
                    fineXScroll = data & 0x07;
                    startAddress &= 0xFFE0;
                    startAddress |= data >> 3;
                }
                else {
                    startAddress &= 0x0FFF;
                    startAddress |= (data & 0x07) << 12;
                    startAddress &= 0xFC1F;
                    startAddress |= (data & 0xF8) << 2;
                }

                firstWrite = !firstWrite;

            };
            cpu.memory.readFunctions[0x2005] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address) {
                return dataLatch;
            };
            cpu.memory.writeFunctions[0x2006] = [&] (
                    MappedMemory<>* const memory,
                    const u16,
                    const u8 data) {
                dataLatch = data;

                if (firstWrite) {
                    startAddress &= 0x00FF;
                    startAddress |= (data & 0x3F) << 8;
                }
                else {
                    startAddress &= 0xFF00;
                    startAddress |= data;
                    address = startAddress;
                }

                firstWrite = !firstWrite;
            };
            cpu.memory.readFunctions[0x2006] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address) {
                return dataLatch;
            };
            cpu.memory.writeFunctions[0x2007] = [&] (
                    MappedMemory<>* const,
                    const u16,
                    const u8 data) {
                memory[address] = dataLatch = data;
                address += verticalPpuaddr ? 32 : 1; 
            };
            cpu.memory.readFunctions[0x2007] = [&] (
                    MappedMemory<>* const,
                    const u16) {
                u8_fast result; 
                if (address >= 0x3F00 && address <= 0x3FFF) {
                    result = memory[address] & grayscaleMask;
                    ppudata = memory[address - 0x1000]; 
                }
                else {
                    result = ppudata;
                    ppudata = memory[address]; 
                }
                address += verticalPpuaddr ? 32 : 1; 
                return result;
            };
            cpu.memory.writeFunctions[0x4014] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                cpu.timer.counter += (513 + cpu.cycle % 2) * 12 - 1;
                for (
                        u16_fast i = data << 8;
                        i < (data << 8) + 0x0100;
                        ++i, ++oamaddr) {
                    primaryOam[oamaddr] = cpu.memory[i];
                }
            };
        }
};
