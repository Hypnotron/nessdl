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
        s8 offset;
        std::vector<std::function<void()>>::const_iterator instrCycle;
        //Value by which instrCycle should be incremented each cycle:
        u8_fast instrCycleStep;

        //Interrupt fields:
        //IRQ level (where > 0 is low):
        u32 irqLevel; 
        //Number of connected IRQ devices:
        u8_fast irqDevices {0};
        //NMI level (where true means edge):
        bool nmiLevel; 
        //Interrupt status (updated before final cycle or manually):
        bool irqPending; 
        bool nmiPending;
        //Toggles default interrupt polling behavior (before final cycle):
        bool defaultInterruptPoll; 

        //General CPU operations (not bound to a specific cycle):
        inline u8 pull() {
            return memory[0x0100 + sp];
        };
        inline void push(const u8 value) {
            memory[0x0100 + sp] = value;
        }
        inline void pollInterrupts() {
            nmiPending = nmiLevel;
            nmiLevel = false;
            irqPending = irqLevel && !(p >> INTERRUPT_DISABLE & 0x01);
        }

        //CPU mnemonic operations:
        const std::function<void()> NUL = [&] () {};
        const std::function<void()> NOP = [&] () {
            value = a; 
        }; 
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
            push(p | 1 << FROM_INSTRUCTION); 
        };
        const std::function<void()> ANC = [&] () {
            a &= value;
            setBit(p, CARRY, a & 0x80);
            setBit(p, NEGATIVE, a & 0x80);
        };
        const std::function<void()> BPL = [&] () {
            value = !(p >> NEGATIVE & 0x01);
        };
        const std::function<void()> CLC = [&] () {
            p &= ~(1 << CARRY);
            value = a;
        };
        const std::function<void()> AND = [&] () {
            a &= value;
            setBit(p, ZERO, a == 0);
            setBit(p, NEGATIVE, a & 0x80);
        };
        const std::function<void()> RLA = [&] () {
            ROL();
            AND();
        };
        const std::function<void()> BIT = [&] () {
            setBit(p, ZERO, (a & value) == 0);
            setBit(p, OVERFLOW, value & 0x40);
            setBit(p, NEGATIVE, value & 0x80);
        };
        const std::function<void()> ROL = [&] () {
            u8_fast oldCarry {static_cast<u8_fast>(p >> CARRY & 0x01)};
            setBit(p, CARRY, value & 0x80);
            value <<= 1;
            setBit(value, 0, oldCarry);

            setBit(p, ZERO, value == 0);
            setBit(p, NEGATIVE, value & 0x80);
        };
        const std::function<void()> PLP = [&] () {
            p = (pull() & 0xEF) | 0x20;
        };
        const std::function<void()> BMI = [&] () {
            value = p >> NEGATIVE & 0x01;
        };
        const std::function<void()> SEC = [&] () {
            p |= 1 << CARRY;
            value = a;
        };
        const std::function<void()> EOR = [&] () {
            a ^= value;
            setBit(p, ZERO, a == 0);
            setBit(p, NEGATIVE, a & 0x80);
        };
        const std::function<void()> SRE = [&] () {
            LSR();
            EOR();
        };
        const std::function<void()> LSR = [&] () {
            setBit(p, CARRY, value & 0x01);
            value >>= 1;
            setBit(p, ZERO, value == 0);
            setBit(p, NEGATIVE, value & 0x80);
        };
        const std::function<void()> PHA = [&] () {
            push(a);
        };
        const std::function<void()> ALR = [&] () {
            a &= value;
            
            value = a;
            LSR();
            a = value;
        };
        const std::function<void()> BVC = [&] () {
            value = !(p >> OVERFLOW & 0x01);
        };
        const std::function<void()> CLI = [&] () {
            p &= ~(1 << INTERRUPT_DISABLE);
            value = a;
        };
        const std::function<void()> ROR = [&] () {
            u8_fast oldCarry {static_cast<u8_fast>(p >> CARRY & 0x01)};
            setBit(p, CARRY, value & 0x01);
            value >>= 1;
            setBit(value, 7, oldCarry);
    
            setBit(p, ZERO, value == 0);
            setBit(p, NEGATIVE, value & 0x80);
        };
        const std::function<void()> RRA = [&] () {
            ROR();
            ADC();
        };
        const std::function<void()> PLA = [&] () {
            a = pull();
            setBit(p, ZERO, a == 0);
            setBit(p, NEGATIVE, a & 0x80);
        }; 
        const std::function<void()> ARR = [&] () {
            AND();
            
            value = a;
            ROR();
            a = value;
        }; 
        const std::function<void()> BVS = [&] () {
            value = p >> OVERFLOW & 0x01; 
        };
        const std::function<void()> SEI = [&] () {
            p |= 1 << INTERRUPT_DISABLE;
            value = a;
        };
        const std::function<void()> STA = [&] () {
            value = a;
        };
        const std::function<void()> SAX = [&] () {
            value = a & x;
        };
        const std::function<void()> STY = [&] () {
            value = y;
        };
        const std::function<void()> STX = [&] () {
            value = x;
        };
        const std::function<void()> DEY = [&] () {
            --y;
            setBit(p, ZERO, y == 0);
            setBit(p, NEGATIVE, y & 0x80);

            value = a;
        };
        const std::function<void()> TXA = [&] () {
            value = a = x;
            setBit(p, ZERO, x == 0);
            setBit(p, NEGATIVE, x & 0x80);
        };
        const std::function<void()> BCC = [&] () {
            value = !(p >> CARRY & 0x01);
        };
        const std::function<void()> TYA = [&] () {
            value = a = y;
        };
        const std::function<void()> TXS = [&] () {
            sp = x;
            value = a;
        };
        const std::function<void()> LDY = [&] () {
            y = value;
            setBit(p, ZERO, y == 0);
            setBit(p, NEGATIVE, y & 0x80);
        };
        const std::function<void()> LDA = [&] () {
            a = value;
            setBit(p, ZERO, a == 0);
            setBit(p, NEGATIVE, a & 0x80);
        };
        const std::function<void()> LDX = [&] () {
            x = value;
            setBit(p, ZERO, x == 0);
            setBit(p, NEGATIVE, x & 0x80);
        };
        const std::function<void()> LAX = [&] () {
            a = x = value;
            setBit(p, ZERO, a == 0);
            setBit(p, NEGATIVE, a & 0x80);
        };
        const std::function<void()> TAY = [&] () {
            y = a;
            setBit(p, ZERO, y == 0);
            setBit(p, NEGATIVE, y & 0x80);

            value = a;
        };
        const std::function<void()> TAX = [&] () {
            x = a;
            setBit(p, ZERO, x == 0);
            setBit(p, NEGATIVE, x & 0x80);

            value = a;
        };
        const std::function<void()> BCS = [&] () {
            value = p >> CARRY & 0x01;
        };
        const std::function<void()> CLV = [&] () {
            p &= ~(1 << OVERFLOW);
            value = a;
        };
        const std::function<void()> TSX = [&] () {
            x = sp;
            setBit(p, ZERO, x == 0);
            setBit(p, NEGATIVE, x & 0x80);

            value = a;
        };
        const std::function<void()> LAS = [&] () {
            a = x = (sp &= value);
            setBit(p, ZERO, a == 0);
            setBit(p, NEGATIVE, a & 0x80);
        }; 
        const std::function<void()> CPY = [&] () {
            u8 tmp = y - value;
            setBit(p, CARRY, y >= value);
            setBit(p, ZERO, tmp == 0); 
            setBit(p, NEGATIVE, tmp & 0x80); 
        }; 
        const std::function<void()> CPX = [&] () {
            u8 tmp = x - value;
            setBit(p, CARRY, x >= value);
            setBit(p, ZERO, tmp == 0);
            setBit(p, NEGATIVE, tmp & 0x80); 
        }; 
        const std::function<void()> CMP = [&] () {
            u8 tmp = a - value;
            setBit(p, CARRY, a >= value);
            setBit(p, ZERO, tmp == 0);
            setBit(p, NEGATIVE, tmp & 0x80); 
        };
        const std::function<void()> INY = [&] () {
            ++y;
            setBit(p, ZERO, y == 0);
            setBit(p, NEGATIVE, y & 0x80);
        
            value = a;
        };
        const std::function<void()> INX = [&] () {
            ++x;
            setBit(p, ZERO, x == 0);
            setBit(p, NEGATIVE, x & 0x80);
        
            value = a;
        };
        const std::function<void()> CLD = [&] () {
            p &= ~(1 << DECIMAL);
            value = a;
        };
        const std::function<void()> SED = [&] () {
            p |= 1 << DECIMAL;
            value = a;
        };
        const std::function<void()> DEX = [&] () {
            --x;
            setBit(p, ZERO, x == 0);
            setBit(p, NEGATIVE, x & 0x80);

            value = a;
        };
        const std::function<void()> DCP = [&] () {
            --value;
            CMP();
        };
        const std::function<void()> DEC = [&] () {
            --value;
            setBit(p, ZERO, value == 0);
            setBit(p, NEGATIVE, value & 0x80);
        };
        const std::function<void()> INC = [&] () {
            ++value;
            setBit(p, ZERO, value == 0);
            setBit(p, NEGATIVE, value & 0x80);
        };
        const std::function<void()> ISC = [&] () {
            ++value;
            SBC();
        };
        const std::function<void()> SBC = [&] () {
            value = ~value;
            ADC();
            value = ~value;
        };
        const std::function<void()> BNE = [&] () {
            value = !(p >> ZERO & 0x01);
        }; 
        const std::function<void()> BEQ = [&] () {
            value = p >> ZERO & 0x01;
        };
        const std::function<void()> SBX = [&] () {
            x &= a;
            setBit(p, CARRY, x >= value);
            x -= value;
            setBit(p, ZERO, x == 0);
            setBit(p, NEGATIVE, x & 0x80);
        };
        const std::function<void()> ADC = [&] () {
            u16 tmp = a + value + (p >> CARRY & 0x01);
            setBit(p, CARRY, tmp > 0xFF);
            setBit(p, ZERO, (tmp & 0xFF) == 0);
            setBit(p, OVERFLOW, (a ^ tmp) & (value ^ tmp) & 0x80); 
            setBit(p, NEGATIVE, tmp & 0x80);
            a = tmp;
        };
            
            
        const std::vector<std::function<void()>> operations {
        /*0*/   NUL, ORA, NUL, SLO, NOP, ORA, ASL, SLO,
                PHP, ORA, ASL, ANC, NOP, ORA, ASL, SLO,
        /*1*/   BPL, ORA, NUL, SLO, NOP, ORA, ASL, SLO,
                CLC, ORA, NOP, SLO, NOP, ORA, ASL, SLO,
        /*2*/   NUL, AND, NUL, RLA, BIT, AND, ROL, RLA,
                PLP, AND, ROL, ANC, BIT, AND, ROL, RLA, 
        /*3*/   BMI, AND, NUL, RLA, NOP, AND, ROL, RLA,
                SEC, AND, NOP, RLA, NOP, AND, ROL, RLA, 
        /*4*/   NUL, EOR, NUL, SRE, NOP, EOR, LSR, SRE,
                PHA, EOR, LSR, ALR, NUL, EOR, LSR, SRE,  
        /*5*/   BVC, EOR, NUL, SRE, NOP, EOR, LSR, SRE,
                CLI, EOR, NOP, SRE, NOP, EOR, LSR, SRE,
        /*6*/   NUL, ADC, NUL, RRA, NOP, ADC, ROR, RRA,
                PLA, ADC, ROR, ARR, NUL, ADC, ROR, RRA,
        /*7*/   BVS, ADC, NUL, RRA, NOP, ADC, ROR, RRA,
                SEI, ADC, NOP, RRA, NOP, ADC, ROR, RRA,
        /*8*/   NOP, STA, NOP, SAX, STY, STA, STX, SAX,
                DEY, NOP, TXA, NUL, STY, STA, STX, SAX,
        /*9*/   BCC, STA, NUL, NUL, STY, STA, STX, SAX,
                TYA, STA, TXS, NUL, NUL, STA, NUL, NUL, 
        /*A*/   LDY, LDA, LDX, LAX, LDY, LDA, LDX, LAX,
                TAY, LDA, TAX, LAX, LDY, LDA, LDX, LAX,
        /*B*/   BCS, LDA, NUL, LAX, LDY, LDA, LDX, LAX,
                CLV, LDA, TSX, LAS, LDY, LDA, LDX, LAX,
        /*C*/   CPY, CMP, NOP, DCP, CPY, CMP, DEC, DCP,
                INY, CMP, DEX, SBX, CPY, CMP, DEC, DCP,
        /*D*/   BNE, CMP, NUL, DCP, NOP, CMP, DEC, DCP,
                CLD, CMP, NOP, DCP, NOP, CMP, DEC, DCP,
        /*E*/   CPX, SBC, NOP, ISC, CPX, SBC, INC, ISC,
                INX, SBC, NOP, SBC, CPX, SBC, INC, ISC,
        /*F*/   BEQ, SBC, NUL, ISC, NOP, SBC, INC, ISC,
                SED, SBC, NOP, ISC, NOP, SBC, INC, ISC, 
        };

        //Miscellanneous commonly used single-cycle lambdas:
        const std::function<void()> dummyReadNextByte = [&] () {
            //Dummy read of the byte after the opcode:
            static_cast<u8>(memory[pc]);
        };
        const std::function<void()> doOp = [&] () {
            operations[opcode]();
        };

        const std::vector<std::vector<std::function<void()
                >>> instrCycles {
            /*0: Interrupt timing */ {
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
                    push(p | !(nmiPending || irqPending) << FROM_INSTRUCTION);
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
                    p = (pull() & 0xEF) | 0x20; 
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
                [&] () {
                    doOp();
                    a = value;
                },
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
                        instrCycleStep = 2; 
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
                        instrCycleStep = 2; 
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
                        instrCycleStep = 2; 
                    }
                    address += x;
                },
                [&] () {
                    //PCH fixup:
                    value = memory[address - 0x0100];
                    instrCycleStep = 2;
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
                        instrCycleStep = 2; 
                    }
                    address += y;
                },
                [&] () {
                    //PCH fixup:
                    value = memory[address - 0x0100];
                    instrCycleStep = 2;
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
                        instrCycleStep = 2;
                    }
                    address += x;
                },
                [&] () {
                    static_cast<u8>(memory[address - 0x0100]);
                    instrCycleStep = 2;
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
                        instrCycleStep = 2;
                    }
                    address += y;
                },
                [&] () {
                    static_cast<u8>(memory[address - 0x0100]);
                    instrCycleStep = 2;
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
                    offset = toSigned(memory[pc++]);
                },
                [&] () {
                    doOp();
                    opcode = memory[pc];
                    if (value) {
                        if (
                                (pc & 0x00FF) + offset <= 0xFFu 
                             && (pc & 0x00FF) + offset >= 0) {
                            //Skip PCH fixup:
                            instrCycleStep = 2; 
                        }
                        else {
                            nmiPending |= nmiLevel;
                            irqPending |= irqLevel 
                                       && (p >> INTERRUPT_DISABLE & 0x01);
                        }
                        pc += offset;
                        return;
                    }
                    ++pc;
                    instrCycle = instrCycles[instrTimings[opcode]].begin();
                    instrCycleStep = 0;
                    defaultInterruptPoll = true;
                },
                [&] () {
                    //PCH fixup:
                    opcode = memory[pc - offset];
                },
                [&] () {
                    opcode = memory[pc++];
                    instrCycle = instrCycles[instrTimings[opcode]].begin();
                    instrCycleStep = 0;
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
                        instrCycleStep = 2;
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
                        instrCycleStep = 2;
                    }
                    address += y;
                },
                [&] () {
                    //PCH fixup:
                    value = memory[address - 0x0100];
                    instrCycleStep = 2;
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
                        instrCycleStep = 2;
                    }
                    address += y;
                },
                [&] () {
                    //PCH fixup:
                    static_cast<u8>(memory[address - 0x0100]);
                    instrCycleStep = 2;
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
            /*35: NUL timing */ {
            },
        };

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
        /*8*/    7, 30,  7, 30, 14, 14, 14, 14,  6,  7,  6, 35, 11, 11, 11, 11, 
        /*9*/   27, 33, 35, 35, 19, 19, 20, 20,  6, 26,  6, 35, 35, 25, 35, 35,
        /*A*/    7, 28,  7, 28, 12, 12, 12, 12,  6,  7,  6,  7,  9,  9,  9,  9,
        /*B*/   27, 31, 35, 31, 15, 15, 16, 16,  6, 22,  6, 22, 21, 21, 22, 22,
        /*C*/    7, 28,  7, 29, 12, 12, 13, 13,  6,  7,  6,  7,  9,  9, 10, 10,
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
            instrCycleStep = 1;
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
            instrCycle += instrCycleStep;
            instrCycleStep = 1;

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

        //TODO: remove 
        void debugOutput() {
            debug::log << "PC: " << static_cast<int>(pc) << "  ";
            debug::log << "OP: " << static_cast<int>(opcode) << "  ";
            debug::log << "VAL: " << static_cast<int>(value) << "  ";
            debug::log << "ADDR: " << address << "  "; 
            debug::log << "A: " << static_cast<int>(a) << "  ";
            debug::log << "X: " << static_cast<int>(x) << "  ";
            debug::log << "Y: " << static_cast<int>(y) << "  ";
            debug::log << "P: " << static_cast<int>(p) << "  ";
            debug::log << "SP: " << static_cast<int>(sp) << "  \n";
        }

};

