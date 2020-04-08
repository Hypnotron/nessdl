#pragma once
#include "byte.hpp"
#include "debug.hpp"
#include "memory.hpp"
#include <functional>

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

        //TODO: General CPU operations (not bound to a specific cycle):
        inline u8 pull() {
            return memory[0x0100 + sp];
        };
        inline void push(const u8 value) {
            memory[0x0100 + sp] = value;
        }

        //TODO: Table of CPU operations (where indices are opcodes):
        const std::vector<std::function<void()>> operations {
               [&] () { 

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {
                
            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {
                
            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }, [&] () {

            }
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
            /*15: Absolute x-indexed read timing */ {
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
            /*16: Absolute y-indexed read timing */ {
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
            /*17: Absolute x-indexed read-modify-write timing */ {
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
            /*18: Absolute y-indexed read-modify-write timing */ {
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
            /*19: Absolute x-indexed write timing */ {
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
            /*20: Absolute y-indexed write timing */ {
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
            /*21: Relative timing */ {
                [&] () {
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
                        pc += address;
                        return;
                    }
                    ++pc;
                    instrCycle = instrCycles[instrTimings[opcode]].begin();
                },
                [&] () {
                    //PCH fixup:
                    opcode = memory[pc - address];
                },
                [&] () {
                    opcode = memory[pc++];
                    instrCycle = instrCycles[instrTimings[opcode]].begin();
                },
            },
            /*22: Pre-indexed read timing */ {
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
            /*23: Pre-indexed read-modify-write timing */ {
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
            /*24: Pre-indexed write timing */ {
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
            /*25: Post-indexed read timing */ {
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
            /*26: Post-indexed read-modify-write timing */ {
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
            /*27: Post-indexed write timing */ {
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
            /*28: JMP indirect timing */ {
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
            }
        };

        //TODO: Timings
        const std::vector<u8_fast> instrTimings {};


    public:
        //Memory:
        MappedMemory<> memory;

        void reset() {
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
                opcode = memory[pc++];
                instrCycle = instrCycles[instrTimings[opcode]].begin();
                return;
            }
            (*instrCycle)();
            ++instrCycle;
        }

};

