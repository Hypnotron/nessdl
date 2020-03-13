#pragma once
#include "byte.hpp"
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
        u16 pointerAddress;
        u16 address;
        std::vector<std::function<void()>>::const_iterator instrCycle {
            instrCycles[opcode].end()
        };

        //TODO: General CPU operations (not bound to a specific cycle):
        inline u8 pull() {
            return memory[0x0100 + sp];
        };
        inline void push(u8 value) {
            memory[0x0100 + sp] = value;
            --sp;
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
                    p = memory[0x0100 + sp];
                    ++sp;
                },
                [&] () {
                    pc = memory[0x0100 + sp];
                    ++sp;
                },
                [&] () {
                    pc |= memory[0x0100 + sp] << 8;
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
                    pc = memory[0x0100 + sp];
                    ++sp;
                },
                [&] () {
                    pc |= memory[0x0100 + sp] << 8;
                },
                [&] () {
                    ++pc;
                },
            },
            /*3: Stack push timing */ {
                dummyReadNextByte,
                [&] () {
                    memory[0x0100 + sp] = opcode == 0x48
                         /*opcode PHA*/ ? a 
                         /*opcode PHP*/ : (p |= (1 << FROM_INSTRUCTION)); 
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
                [&] () {
                    operations[opcode]();
                }
            },
            /*5: JSR timing */ {
                [&] () {
                    address = memory[pc];
                    ++pc;
                },
                [] () {
                    //DISCREPANCY: idek what this cycle is
                },
                [&] () {
                    memory[0x0100 + sp] = pc >> 8;
                    --sp;
                },
                [&] () {
                    memory[0x0100 + sp] = pc & 0x00FF;
                    --sp;
                },
                [&] () {
                    pc = memory[pc] << 8 | address;
                },
            },
            /*6: Im TODO */

        };

        //TODO: Timings
        std::vector<u8_fast> instrTimings {};


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
                opcode = memory[pc];
                ++pc;

                instrCycle = instrCycles[instrTimings[opcode]].begin();
                return;
            }
            (*instrCycle)();
            ++instrCycle;
        }

};

