#pragma once
#include <cassert>
#include <functional>
#include "debug.hpp"
#include "byte.hpp"
#include "memory.hpp"

class Cpu {
    //TODO: Power-up state
    private:
        //General-purpose registers:
        u8 a {0}, x {0}, y {0};
        //Program counter:
        u16 pc;
        //Stack pointer:
        u8 sp {0};
        //Flags:
        u8 p {0x34};
        enum Flag : u8 {
            CARRY,
            ZERO,
            INTERRUPT_DISABLE,
            DECIMAL, //ignored on the 2A03
            FROM_INSTRUCTION,
            FLAG5, //always 1
            OVERFLOW,
            NEGATIVE
        };

        //Per-instruction temporary registers:
        u8 opcode;
        u8 value;
        u8 pointerAddress;
        u8 pointerAddressHigh;
        u16 address;
        std::vector<std::function<void()>>::const_iterator instrCycle;

        //Interrupt fields:
        //IRQ level (where > 0 is low):
        u32 irqLevel; 
        u8_fast irqDevices;
        //NMI level (where true means edge):
        bool nmiLevel; 
        //Interrupt status (updated before final cycle or manually):
        bool irqPending; 
        bool nmiPending;
        //Toggles default interrupt polling behavior (before final cycle):
        bool defaultInterruptPoll; 

        //TODO: General CPU operations (not bound to a specific cycle):
        inline u8 pull() {
            return memory[0x0100 + sp];
        };
        inline void push(const u8 value) {
            memory[0x0100 + sp] = value;
        }
        inline void pollInterrupts() {
            nmiPending = nmiLevel;
            nmiLevel = false;
            irqPending = irqLevel && ~(p >> INTERRUPT_DISABLE & 0x01);
        }

        //TODO: Specialized CPU operations:
        const std::function<void()> NOP = [&] () {};
        const std::function<void()> ORA = [&] () {
            a |= value;
            setBit(p, ZERO, a == 0);
            setBit(p, NEGATIVE, a & 0x80);
        };
        const std::function<void()> SLO = [&] () {
            setBit(p, CARRY, value & 0x80);
            value <<= 1;
            a |= value; 
            setBit(p, ZERO, a == 0);
            setBit(p, NEGATIVE, a & 0x80);
        };
        const std::function<void()> ASL = [&] () {
            setBit(p, CARRY, value & 0x80);
            value <<= 1;
            setBit(p, ZERO, value == 0);
            setBit(p, NEGATIVE, value & 0x80);
        };
        const std::function<void()> PHP = [&] () {
            setBit(p, FROM_INSTRUCTION, 1);
            push(p); 
        };
        const std::function<void()> ANC = [&] () {
            a &= value;
            setBit(p, NEGATIVE, a & 0x80);
            setBit(p, CARRY, a & 0x80);
        };
            

        

        //TODO: Table of CPU operations (where indices are opcodes):
        const std::vector<std::function<void()>> operations {
        };

        //Miscellanneous commonly used single-cycle lambdas:
        const std::function<void()> dummyReadNextByte = [&] () {
            //Dummy read of the byte after the opcode:
            static_cast<u8>(memory[pc]);
        };
        const std::function<void()> doOp = [&] () {
            operations[opcode]();
        };

        //TODO: Individual cycle breakdown for each instruction:
        const std::vector<std::vector<std::function<void()
                >>> instrCycles {
            /*0: TODO: Interrupt timing */ {
                [&] () {
                    if (!(nmiPending || irqPending)) {
                        static_cast<u8>(memory[pc++]);
                    }
                },
                [&] () {
                    push(pc >> 8);
                    --sp;
                },
                [&] () {
                    push(pc & 0x00FF);
                    --sp;
                },
                [&] () {
                    setBit(p, FROM_INSTRUCTION, !(nmiPending || irqPending));
                    push(p);
                    --sp;

                    pollInterrupts();                    
                },
                [&] () {
                    address = nmiPending ? 0xFFFA : 0xFFFE;
                    pc = memory[address++];

                    p |= 1 << INTERRUPT_DISABLE;
                    nmiPending = false;
                },
                [&] () {
                    pc |= memory[address] << 8;
                }, 
            },
            /*1: RTI timing: */ {
                dummyReadNextByte,
                [&] () {
                    //DISCREPANCY: Unsure if the 6502 dummy reads 
                    //from (0x0100 + sp) or not, so not doing it 
                    //static_cast<u8>(memory[0x0100 + sp]);
                    ++sp;
                },
                [&] () {
                    p = pull(); 
                    ++sp;
                },
                [&] () {
                    pc = pull();
                    ++sp; 
                },
                [&] () {
                    pc |= pull() << 8;
                },
            },
            /*2: RTS timing */ {
                dummyReadNextByte,
                [&] () {
                    //DISCREPANCY: Unsure if the 6502 dummy reads 
                    //from (0x0100 + sp) or not, so not doing it 
                    //static_cast<u8>(memory[0x0100 + sp]);
                    ++sp;
                },
                [&] () {
                    pc = pull();
                    ++sp; 
                },
                [&] () {
                    pc |= pull() << 8; 
                },
                [&] () {
                    ++pc;
                },
            },
            /*3: Stack push timing */ {
                dummyReadNextByte,
                [&] () {
                    doOp();
                    --sp;
                },
            },
            /*4: Stack pull timing */ {
                dummyReadNextByte,
                [&] () {
                    //DISCREPANCY: Unsure if the 6502 dummy reads 
                    //from (0x0100 + sp) or not, so not doing it 
                    //static_cast<u8>(memory[0x0100 + sp]);
                    ++sp;
                },
                doOp,
            },
            /*5: JSR timing */ {
                [&] () {
                    address = memory[pc++];
                },
                [] () {
                    //DISCREPANCY: idek what this cycle is
                },
                [&] () {
                    push(pc >> 8);
                    --sp;
                },
                [&] () {
                    push(pc & 0x00FF);
                    --sp; 
                },
                [&] () {
                    pc = memory[pc] << 8 | address;
                },
            },
            /*6: Implied timing */ {
                doOp,
            },
            /*7: Immediate timing */ {
                [&] () {
                    value = memory[pc++];
                    doOp();
                },
            }, 
            /*8: Absolute JMP timing */ {
                [&] () {
                    address = memory[pc++];
                },
                [&] () {
                    pc = memory[pc] << 8 | address;
                },
            },
            /*9: Absolute read timing */ {
                [&] () {
                    address = memory[pc++];
                },
                [&] () {
                    address |= memory[pc++] << 8;
                },
                [&] () {
                    value = memory[address];
                    doOp();
                },
            },
            /*10: Absolute read-modify-write timing */ {
                [&] () {
                    address = memory[pc++];
                },
                [&] () {
                    address |= memory[pc++] << 8;
                },
                [&] () {
                    value = memory[address];
                },
                [&] () {
                    memory[address] = value;
                    doOp();
                },
                [&] () {
                    memory[address] = value;
                },
            },
            /*11: Absolute write timing */ {
                [&] () {
                    address = memory[pc++];
                },
                [&] () {
                    address |= memory[pc++] << 8; 
                },
                [&] () {
                    doOp();
                    memory[address] = value;
                },
            },
            /*12: Zero page read timing */ {
                [&] () {
                    address = memory[pc++];
                },
                [&] () {
                    value = memory[address];
                    doOp();
                },
            },
            /*13: Zero page read-modify-write timing */ {
                [&] () {
                    address = memory[pc++];
                },
                [&] () {
                    value = memory[address];
                },
                [&] () {
                    memory[address] = value;
                    doOp();
                },
                [&] () {
                    memory[address] = value;
                },
            },
            /*14: Zero page write timing */ {
                [&] () {
                    address = memory[pc++];
                },
                [&] () {
                    doOp();
                    memory[address] = value;
                },
            },
            /*15: Zero page x-indexed read timing */ {
                [&] () {
                    address = memory[pc++];
                },
                [&] () {
                    value = memory[address];
                    address += x;
                    address &= 0xFF;
                },
                [&] () {
                    value = memory[address];
                    doOp();
                },
            },
            /*16: Zero page y-indexed read timing */ {
                [&] () {
                    address = memory[pc++];
                },
                [&] () {
                    value = memory[address];
                    address += y;
                    address &= 0xFF;
                },
                [&] () {
                    value = memory[address];
                    doOp();
                },
            },
            /*17: Zero page x-indexed read-modify-write timing */ {
                [&] () {
                    address = memory[pc++];
                },
                [&] () {
                    value = memory[address];
                    address += x;
                    address &= 0xFF;
                },
                [&] () {
                    value = memory[address];
                },
                [&] () {
                    memory[address] = value;
                    doOp();
                },
                [&] () {
                    memory[address] = value;
                },
            },
            /*18: Zero page y-indexed read-modify-write timing */ {
                [&] () {
                    address = memory[pc++];
                },
                [&] () {
                    value = memory[address];
                    address += y;
                    address &= 0xFF;
                },
                [&] () {
                    value = memory[address];
                },
                [&] () {
                    memory[address] = value;
                    doOp();
                },
                [&] () {
                    memory[address] = value;
                },
            },
            /*19: Zero page x-indexed write timing */ {
                [&] () {
                    address = memory[pc++];
                },
                [&] () {
                    static_cast<u8>(memory[address]);
                    address += x;
                    address &= 0xFF;
                },
                [&] () {
                    doOp();
                    memory[address] = value;
                },
            },
            /*20: Zero page y-indexed write timing */ {
                [&] () {
                    address = memory[pc++];
                },
                [&] () {
                    static_cast<u8>(memory[address]);
                    address += y;
                    address &= 0xFF;
                },
                [&] () {
                    doOp();
                    memory[address] = value;
                },
            },
            /*21: Absolute x-indexed read timing */ {
                [&] () {
                    address = memory[pc++];
                },
                [&] () {
                    address |= memory[pc++] << 8;
                    if ((address & 0x00FF) + x <= 0xFF) {
                        //Skip PCH fixup: 
                        ++instrCycle; 
                    }
                    address += x;
                },
                //PCH fixup:
                [&] () {
                    value = memory[address - 0x0100];
                },
                [&] () {
                    value = memory[address];
                    doOp();
                }, 
            },
            /*22: Absolute y-indexed read timing */ {
                [&] () {
                    address = memory[pc++];
                },
                [&] () {
                    address |= memory[pc++] << 8;
                    if ((address & 0x00FF) + y <= 0xFF) {
                        //Skip PCH fixup: 
                        ++instrCycle; 
                    }
                    address += y;
                },
                //PCH fixup:
                [&] () {
                    value = memory[address - 0x0100];
                },
                [&] () {
                    value = memory[address];
                    doOp();
                }, 
            },
            /*23: Absolute x-indexed read-modify-write timing */ {
                [&] () {
                    address = memory[pc++];
                },
                [&] () {
                    address |= memory[pc++] << 8;
                    if ((address & 0x00FF) + x <= 0xFF) {
                        //Skip true PCH fixup: 
                        ++instrCycle; 
                    }
                    address += x;
                },
                [&] () {
                    //PCH fixup:
                    value = memory[address - 0x0100];
                    ++instrCycle;
                },
                [&] () {
                    //Dummy PCH fixup:
                    value = memory[address];
                },
                [&] () {
                    value = memory[address];
                },
                [&] () {
                    memory[address] = value;
                    doOp();
                },
                [&] () {
                    memory[address] = value; 
                },
            },
            /*24: Absolute y-indexed read-modify-write timing */ {
                [&] () {
                    address = memory[pc++];
                },
                [&] () {
                    address |= memory[pc++] << 8;
                    if ((address & 0x00FF) + y <= 0xFF) {
                        //Skip true PCH fixup: 
                        ++instrCycle; 
                    }
                    address += y;
                },
                [&] () {
                    //PCH fixup:
                    value = memory[address - 0x0100];
                    ++instrCycle;
                },
                [&] () {
                    //Dummy PCH fixup:
                    value = memory[address];
                },
                [&] () {
                    value = memory[address];
                },
                [&] () {
                    memory[address] = value;
                    doOp();
                },
                [&] () {
                    memory[address] = value; 
                },
            },
            /*25: Absolute x-indexed write timing */ {
                [&] () {
                    address = memory[pc++];
                },
                [&] () {
                    address |= memory[pc++] << 8;
                    if ((address & 0x00FF) + x <= 0xFF) {
                        //Skip PCH fixup: 
                        ++instrCycle;
                    }
                    address += x;
                },
                [&] () {
                    static_cast<u8>(memory[address - 0x0100]);
                    ++instrCycle;
                },
                [&] () {
                    static_cast<u8>(memory[address]);
                },
                [&] () {
                    doOp();
                    memory[address] = value;
                },
            },
            /*26: Absolute y-indexed write timing */ {
                [&] () {
                    address = memory[pc++];
                },
                [&] () {
                    address |= memory[pc++] << 8;
                    if ((address & 0x00FF) + y <= 0xFF) {
                        //Skip PCH fixup: 
                        ++instrCycle;
                    }
                    address += y;
                },
                [&] () {
                    static_cast<u8>(memory[address - 0x0100]);
                    ++instrCycle;
                },
                [&] () {
                    static_cast<u8>(memory[address]);
                },
                [&] () {
                    doOp();
                    memory[address] = value;
                },
            },
            /*27: Relative timing */ {
                [&] () {
                    defaultInterruptPoll = false;
                    address = toSigned(memory[pc++]);
                },
                [&] () {
                    doOp();
                    opcode = memory[pc];
                    if (value) {
                        if ((pc & 0x00FF) + address <= 0xFF) {
                            //Skip PCH fixup:
                            ++instrCycle; 
                        }
                        else {
                            nmiPending |= nmiLevel;
                            irqPending |= irqLevel 
                                       && (p >> INTERRUPT_DISABLE & 0x01);
                        }
                        pc += address;
                        return;
                    }
                    ++pc;
                    instrCycle = instrCycles[instrTimings[opcode]].begin();
                    defaultInterruptPoll = true;
                },
                [&] () {
                    //PCH fixup:
                    opcode = memory[pc - address];
                },
                [&] () {
                    opcode = memory[pc++];
                    instrCycle = instrCycles[instrTimings[opcode]].begin();
                    defaultInterruptPoll = true;
                },
            },
            /*28: Pre-indexed read timing */ {
                [&] () {
                    pointerAddress = memory[pc++];
                },
                [&] () {
                    static_cast<u8>(memory[pointerAddress]);
                    pointerAddress += x;
                },
                [&] () {
                    address = memory[pointerAddress++];
                },
                [&] () {
                    address |= memory[pointerAddress] << 8;
                },
                [&] () {
                    value = memory[address];
                    doOp();
                },
            },
            /*29: Pre-indexed read-modify-write timing */ {
                [&] () {
                    pointerAddress = memory[pc++];
                },
                [&] () {
                    static_cast<u8>(memory[pointerAddress]);
                    pointerAddress += x;
                },
                [&] () {
                    address = memory[pointerAddress++];
                },
                [&] () {
                    address |= memory[pointerAddress] << 8;
                },
                [&] () {
                    value = memory[address];
                },
                [&] () {
                    memory[address] = value;
                    doOp();
                },
                [&] () {
                    memory[address] = value;
                },
            },
            /*30: Pre-indexed write timing */ {
                [&] () {
                    pointerAddress = memory[pc++];
                },
                [&] () {
                    static_cast<u8>(memory[pointerAddress]);
                    pointerAddress += x; 
                },
                [&] () {
                    address = memory[pointerAddress++];
                },
                [&] () {
                    address |= memory[pointerAddress] << 8;
                },
                [&] () {
                    doOp();
                    memory[address] = value;
                },
            },
            /*31: Post-indexed read timing */ {
                [&] () {
                    pointerAddress = memory[pc++];
                },
                [&] () {
                    address = memory[pointerAddress++];
                },
                [&] () {
                    address |= memory[pointerAddress] << 8;
                    if ((address & 0x00FF) + y <= 0xFF) {
                        //Skip PCH fixup:
                        ++instrCycle;
                    }
                    address += y;
                },
                [&] () {
                    //PCH fixup:
                    value = memory[address - 0x0100];
                },
                [&] () {
                    value = memory[address];
                    doOp();
                },
            },
            /*32: Post-indexed read-modify-write timing */ {
                [&] () {
                    pointerAddress = memory[pc++];
                },
                [&] () {
                    address = memory[pointerAddress++]; 
                },
                [&] () {
                    address |= memory[pointerAddress] << 8;
                    if ((address & 0x00FF) + y <= 0xFF) {
                        //Skip true PCH fixup:
                        ++instrCycle;
                    }
                    address += y;
                },
                [&] () {
                    //PCH fixup:
                    value = memory[address - 0x0100];
                    ++instrCycle;
                },
                [&] () {
                    //Dummy PCH fixup:
                    value = memory[address];
                },
                [&] () {
                    value = memory[address];
                },
                [&] () {
                    memory[address] = value;
                    doOp();
                },
                [&] () {
                    memory[address] = value;
                },
            },
            /*33: Post-indexed write timing */ {
                [&] () {
                    pointerAddress = memory[pc++];
                },
                [&] () {
                    address = memory[pointerAddress++];
                },
                [&] () {
                    address |= memory[pointerAddress] << 8;
                    if ((address & 0x00FF) + y <= 0xFF) {
                        //Skip PCH fixup:
                        ++instrCycle;
                    }
                    address += y;
                },
                [&] () {
                    //PCH fixup:
                    static_cast<u8>(memory[address - 0x0100]);
                    ++instrCycle;
                },
                [&] () {
                    //Dummy PCH fixup:
                    static_cast<u8>(memory[address]);
                },
                [&] () {
                    doOp();
                    memory[address] = value;
                },
            }, 
            /*34: JMP indirect timing */ {
                [&] () {
                    pointerAddress = memory[pc++];
                },
                [&] () {
                    pointerAddressHigh = memory[pc++];
                },
                [&] () {
                    address = memory[
                            pointerAddressHigh << 8 | pointerAddress++];
                },
                [&] () {
                    pc = memory[
                            pointerAddressHigh << 8 | pointerAddress 
                            ] << 8 | address;
                },
            },
            /*35: NOP timing */ {
            },
        };

        //TODO: Timings
        const std::vector<u8_fast> instrTimings {
        //      .0, .1, .2, .3, .4, .5, .6, .7, .8, .9, .A, .B, .C, .D, .E, .F, 
        /*0*/    0, 28, 35, 29, 12, 12, 13, 13,  3,  7,  6,  7,  9,  9, 10, 10, 
        /*1*/   27, 31, 35, 32, 15, 15, 17, 17,  6, 22,  6, 24, 21, 21, 23, 23,  
        /*2*/    5, 28, 35, 29, 12, 12, 13, 13,  4,  7,  6,  7,  9,  9, 10, 10, 
        /*3*/   27, 31, 35, 32, 15, 15, 17, 17,  6, 22,  6, 24, 21, 21, 23, 23,
        /*4*/    1, 28, 35, 29, 12, 12, 13, 13,  3,  7,  6,  7,  8,  9, 10, 10,
        /*5*/   27, 31, 35, 32, 15, 15, 17, 17,  6, 22,  6, 24, 21, 21, 23, 23, 
        /*6*/    2, 28, 35, 29, 12, 12, 13, 13,  4,  7,  6,  7, 34,  9, 10, 10,
        /*7*/   27, 31, 35, 32, 15, 15, 17, 17,  6, 22,  6, 24, 21, 21, 23, 23,
        /*8*/    7, 30,  7, 30, 14, 14, 14, 14,  6,  7,  6,  7, 11, 11, 11, 11, 
        /*9*/   27, 33, 35, 33, 19, 19, 20, 20,  6, 26,  6, 26, 25, 25, 26, 26,
        /*A*/    7, 28,  7, 28, 12, 12, 12, 12,  6,  7,  6,  7,  9,  9,  9,  9,
        /*B*/   27, 31, 35, 31, 15, 15, 16, 16,  6, 22,  6, 22, 21, 21, 22, 22,
        /*C*/    7, 28,  7, 29, 12, 12, 13, 13,  6,  7,  6,  7,  9,  9,  9,  9,
        /*D*/   27, 31, 35, 32, 15, 15, 17, 17,  6, 22,  6, 24, 21, 21, 23, 23,
        /*E*/    7, 28,  7, 29, 12, 12, 13, 13,  6,  7,  6,  7,  9,  9, 10, 10,
        /*F*/   27, 31, 35, 32, 15, 15, 17, 17,  6, 22,  6, 24, 21, 21, 23, 23,
        };


    public:
        //Memory:
        MappedMemory<> memory;

        void reset() {
            defaultInterruptPoll = true;
            nmiPending = false;
            irqPending = false;
            nmiLevel = false;
            irqLevel = 0;

            sp -= 3; //dummy stack write
            p |= 0x04; //set interrupt disable 
            pc = readBytes<2, u16>(memory.begin() + 0xFFFC);

            //Invalidate the next cycle iterator so 
            //that an opcode fetch occurs next tick:
            instrCycle = instrCycles[instrTimings[opcode]].end();
        } 

        void tick() {
            //Fetch next opcode if instruction has finished: 
            if (instrCycle == instrCycles[instrTimings[opcode]].end()) {
                opcode = (nmiPending || irqPending ? 0 : memory[pc++]);
                instrCycle = instrCycles[instrTimings[opcode]].begin();

                //TODO: Better solution for relative branching
                if (instrCycle == instrCycles[27].begin()) {
                    pollInterrupts();
                }

                return;
            }

            (*instrCycle)();
            ++instrCycle;

            if (
                    instrCycle == instrCycles[instrTimings[opcode]].end() - 1
                 && defaultInterruptPoll) { 
                pollInterrupts();
            }
        }

        u8_fast connectIrq() {
            assert(irqDevices < 32 
                    && "Cannot allocate more than 32 IRQ devices.");
            return irqDevices++;
        }
        void pullIrq(u8_fast bit) {
            irqLevel |= 1 << bit;
        }
        void releaseIrq(u8_fast bit) {
            irqLevel &= ~(1 << bit);
        }
        void edgeNmi() {
            nmiLevel = true;
        }
};

