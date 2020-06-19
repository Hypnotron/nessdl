#pragma once
#include <cassert>
#include <array>
#include <vector>
#include <functional>
//TODO: remove debug module
#include "debug.hpp"
#include "byte.hpp"
#include "counter.hpp"
#include "memory.hpp"

class Cpu {
    private:
        //General-purpose registers:
        u8 a {0}, x {0}, y {0};
        //Program counter:
        u16 pc;
        //Stack pointer:
        u8 sp {0};
        //Flags:
        u8 p {0x24};
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
        //Set to true by certain instructions to skip interrupt polling:
        bool doNotInterrupt {false};

        //General CPU operations (not bound to a specific cycle):
        inline u8 pull() {
            return memory[0x0100 + sp];
        };
        inline void push(const u8 value) {
            memory[0x0100 + sp] = value;
        }
        inline void pollInterrupts() {
            nmiPending |= nmiLevel;
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
            setBit(p, ZERO, a == 0); 
            setBit(p, NEGATIVE, a & 0x80);
        };
        const std::function<void()> BPL = [&] () {
            value = !(p >> NEGATIVE & 0x01);
        };
        const std::function<void()> CLC = [&] () {
            p &= ~(1 << CARRY);
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
            
            setBit(p, CARRY, a & 0x40);
            setBit(p, OVERFLOW, ((a >> 5) ^ (a >> 6)) & 0x01);
        }; 
        const std::function<void()> BVS = [&] () {
            value = p >> OVERFLOW & 0x01; 
        };
        const std::function<void()> SEI = [&] () {
            p |= 1 << INTERRUPT_DISABLE;
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
        };
        const std::function<void()> TXA = [&] () {
            value = x;
            setBit(p, ZERO, x == 0);
            setBit(p, NEGATIVE, x & 0x80);
        };
        const std::function<void()> BCC = [&] () {
            value = !(p >> CARRY & 0x01);
        };
        const std::function<void()> TYA = [&] () {
            value = y;
            setBit(p, ZERO, y == 0);
            setBit(p, NEGATIVE, y & 0x80);
        };
        const std::function<void()> TXS = [&] () {
            sp = x;
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
        };
        const std::function<void()> TAX = [&] () {
            x = a;
            setBit(p, ZERO, x == 0);
            setBit(p, NEGATIVE, x & 0x80);
        };
        const std::function<void()> BCS = [&] () {
            value = p >> CARRY & 0x01;
        };
        const std::function<void()> CLV = [&] () {
            p &= ~(1 << OVERFLOW);
        };
        const std::function<void()> TSX = [&] () {
            x = sp;
            setBit(p, ZERO, x == 0);
            setBit(p, NEGATIVE, x & 0x80);
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
        };
        const std::function<void()> INX = [&] () {
            ++x;
            setBit(p, ZERO, x == 0);
            setBit(p, NEGATIVE, x & 0x80);
        };
        const std::function<void()> CLD = [&] () {
            p &= ~(1 << DECIMAL);
        };
        const std::function<void()> SED = [&] () {
            p |= 1 << DECIMAL;
        };
        const std::function<void()> DEX = [&] () {
            --x;
            setBit(p, ZERO, x == 0);
            setBit(p, NEGATIVE, x & 0x80);
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
            
            
        const std::array<std::function<void()>, 0x100> operations {
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
        const std::function<void()> fetchOp = [&] () {
            debugOutput();

            opcode = (nmiPending || irqPending) ? 0 : memory[pc++];
            instrCycle = instrCycles[instrTimings[opcode]].begin();
            instrCycleStep = 0;
        };

        const std::vector<std::vector<std::function<void()>>> instrCycles {
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

                    doNotInterrupt = nmiPending || irqPending;

                    nmiPending = false;
                    irqPending = false;
                },
                [&] () {
                    pc |= memory[address] << 8;
                    doNotInterrupt = false;
                }, 
                fetchOp,
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
                fetchOp,
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
                fetchOp,
            },
            /*3: Stack push timing */ {
                dummyReadNextByte,
                [&] () {
                    doOp();
                    --sp;
                },
                fetchOp,
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
                fetchOp,
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
                fetchOp,
            },
            /*6: Implied timing */ {
                [&] () {
                    value = a;
                    doOp();
                    a = value;
                },
                fetchOp,
            },
            /*7: Immediate timing */ {
                [&] () {
                    value = memory[pc++];
                    doOp();
                },
                fetchOp,
            }, 
            /*8: Absolute JMP timing */ {
                [&] () {
                    address = memory[pc++];
                },
                [&] () {
                    pc = memory[pc] << 8 | address;
                },
                fetchOp,
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
                fetchOp,
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
                fetchOp,
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
                fetchOp,
            },
            /*12: Zero page read timing */ {
                [&] () {
                    address = memory[pc++];
                },
                [&] () {
                    value = memory[address];
                    doOp();
                },
                fetchOp,
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
                fetchOp,
            },
            /*14: Zero page write timing */ {
                [&] () {
                    address = memory[pc++];
                },
                [&] () {
                    doOp();
                    memory[address] = value;
                },
                fetchOp,
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
                fetchOp,
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
                fetchOp,
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
                fetchOp,
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
                fetchOp,
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
                fetchOp,
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
                fetchOp,
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
                fetchOp,
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
                fetchOp,
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
                fetchOp,
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
                fetchOp,
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
                fetchOp,
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
                fetchOp,
            },
            /*27: Relative timing */ {
                [&] () {
                    offset = toSigned(memory[pc++]);
                },
                [&] () {
                    doOp();
                    debugOutput();
                    opcode = (nmiPending || irqPending) ? 0 : memory[pc];
                    if (value) {
                        if (
                                (pc & 0x00FF) + offset <= 0xFFu 
                             && (pc & 0x00FF) + offset >= 0) {
                            //Skip PCH fixup:
                            instrCycleStep = 2; 
                            doNotInterrupt = true;
                        }
                        else {
                            nmiPending |= nmiLevel;
                            nmiLevel = false;
                            irqPending |= irqLevel 
                                       && !(p >> INTERRUPT_DISABLE & 0x01);
                        }
                        pc += offset;
                        return;
                    }
                    pc += !(nmiPending || irqPending);
                    instrCycle = instrCycles[instrTimings[opcode]].begin();
                    instrCycleStep = 0;
                    doNotInterrupt = false;
                },
                [&] () {
                    //PCH fixup:
                    opcode = memory[pc - offset];
                    doNotInterrupt = true;
                },
                [&] () {
                    opcode = (nmiPending || irqPending) ? 0 : memory[pc++];
                    instrCycle = instrCycles[instrTimings[opcode]].begin();
                    instrCycleStep = 0;
                    doNotInterrupt = false;
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
                fetchOp,
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
                fetchOp,
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
                fetchOp,
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
                fetchOp,
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
                fetchOp,
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
                fetchOp,
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
                fetchOp,
            },
            /*35: NUL timing */ {
                fetchOp,
            },
        };

        const std::array<u8_fast, 0x100> instrTimings {
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
        //Tick counter:
        u32_fast cycle {0};

        //Memory:
        MappedMemory<> memory{0};

        void reset() {
            doNotInterrupt = false;
            nmiPending = false;
            irqPending = false;
            nmiLevel = false;
            irqLevel = 0;

            sp -= 3; //dummy stack write
            p |= 0x04; //set interrupt disable 
            pc = readBytes<2, u16>(memory.begin() + 0xFFFC);

            //Set the next cycle iterator to an arbitrary opcode fetch:
            instrCycle = instrCycles[0].end() - 1;
            instrCycleStep = 1;

            timer.counter += 7 * (timer.reload + 1) - 1;
            cycle = 0;

            //TODO: remove hex output
            debug::log << std::hex;
        } 

        Counter<s16_fast> timer{0, [&] () {
            (*instrCycle)();
            instrCycle += instrCycleStep;
            instrCycleStep = 1;

            if (
                    (instrCycle == instrCycles[27].begin()
                 || instrCycle == instrCycles[instrTimings[opcode]].end() - 2)
                 && !doNotInterrupt) {
                pollInterrupts();
            }

            ++cycle;
        }};

        Cpu() {
            memory.readFunctions[0x1FFF] = [] (
                    MappedMemory<>* const memory, 
                    const u16 address) {
                return memory->memory[address & 0x07FF];
            };
            memory.writeFunctions[0x1FFF] = [] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                memory->memory[address & 0x07FF] = data;
            };
        }

        void tick(const u8_fast ticks = 1) {
            timer.tick(ticks);
        }

        template <typename StateType>
        void dumpState(StateType& state) {
            auto dump {[&] (const u8 data) {
                //                  size in bytes
                state.write(&data,              1);
            }};

            dump(a);
            dump(x);
            dump(y);
            dump(pc & 0x00FF);
            dump(pc >> 8);
            dump(sp);
            dump(p);
            
            {
                u8 originalOpcode =
                        (instrCycle == instrCycles[27].begin() + 2
                     || instrCycle == instrCycles[27].begin() + 3) 
                      ? 27
                      : opcode;
                dump(originalOpcode);
                dump(opcode);
                dump(value);
                dump(pointerAddress);
                dump(pointerAddressHigh);
                dump(address & 0x00FF);
                dump(address >> 8);
                dump(offset);
                dump(
                        instrCycle 
                      - instrCycles[instrTimings[originalOpcode]].begin());
                dump(instrCycleStep);
            }

            dump(irqLevel & 0x000000FF);
            dump(irqLevel >> 8 & 0x0000FF);
            dump(irqLevel >> 16 & 0x00FF);
            dump(irqLevel >> 24);
            dump(irqDevices);
            dump(nmiLevel);
            dump(irqPending);
            dump(nmiPending);
            dump(doNotInterrupt);

            dump(cycle & 0x000000FF);
            dump(cycle >> 8 & 0x0000FF);
            dump(cycle >> 16 & 0x00FF);
            dump(cycle >> 24);
            dump(timer.reload & 0x00FF);
            dump(timer.reload >> 8);
            dump(timer.counter & 0x00FF);
            dump(timer.counter >> 8);

            state.write(reinterpret_cast<const char*>(
                    memory.memory.data()),
                    0x0800);
        }
        template <typename StateType>
        void loadState(StateType& state) {
            auto load {[&] () {
                u8 result;
                //                   size in bytes
                state.read(&result,              1);
                return result;
            }};
            
            a = load();
            x = load();
            y = load();
            pc = load();
            pc |= load() << 8;
            sp = load();
            p = load();

            {
                u8 originalOpcode = load();
                opcode = load();
                value = load();
                pointerAddress = load();
                pointerAddressHigh = load();
                address = load();
                address |= load() << 8;
                offset = toSigned(load());
                instrCycle = 
                        instrCycles[instrTimings[originalOpcode]].begin() 
                      + load();
                instrCycleStep = load();
            }

            irqLevel = load();
            irqLevel |= load() << 8;
            irqLevel |= load() << 16;
            irqLevel |= load() << 24;
            irqDevices = load();
            nmiLevel = load();
            irqPending = load();
            nmiPending = load();
            doNotInterrupt = load();

            cycle = load();
            cycle |= load() << 8;
            cycle |= load() << 16;
            cycle |= load() << 24;

            u16 tmp = load();
            tmp |= load() << 8;
            timer.reload = toSigned(tmp); 

            tmp = load();
            tmp |= load() << 8;
            timer.counter = toSigned(tmp); 

            state.read(reinterpret_cast<char*>(
                    memory.memory.data()), 
                    0x0800);
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
        bool isPullingIrq(u8_fast bit) {
            return (irqLevel >> bit) & 1;
        }
        void edgeNmi() {
            nmiLevel = true;
        }

        //TODO: remove debug log function 
        void debugOutput () const {
            debug::log << "IRQ: " << irqPending << "  ";
            debug::log << "NMI: " << nmiPending << "  ";
            debug::log << "OP: " << static_cast<int>(opcode) << "  ";
            debug::log << "VAL: " << static_cast<int>(value) << "  ";
            debug::log << "ADDR: " << address << "  "; 
            debug::log << "A: " << static_cast<int>(a) << "  ";
            debug::log << "X: " << static_cast<int>(x) << "  ";
            debug::log << "Y: " << static_cast<int>(y) << "  ";
            debug::log << "P: " << static_cast<int>(p) << "  ";
            debug::log << "SP: " << static_cast<int>(sp) << "  \n";
            debug::log << "PC: " << std::setfill('0') << std::setw(4) 
                       << pc << "  ";
        }
};

class Apu {
    private:
        //Sound units belonging to individual channels:
        struct LengthCounter {
            bool enabled {false}; 
            bool halt {false};

            Counter<s16_fast> counter{0, [&] () {
                 counter.counter = 0;
                 return true;
            }};

            void tick() {
                if (!halt) {
                    counter.tick();
                }
            } 

            void toggle(const bool enable) {
                enabled = enable;
                if (!enabled) {
                    counter.counter = 0;
                }
            }
        };
        struct LinearCounter {
            bool reload {true};
            bool control {false};       
            
            Counter<s16_fast> counter{0, [&] () {
                counter.counter = 0;
                return true;
            }};

            void tick() {
                counter.tick();
                if (reload) {
                    counter.counter = counter.reload;
                }
             
                if (!control) {
                    reload = false;
                }
            }

            bool isNonZero() const {
                return counter.counter > 0;
            }
        };
        struct Sweep {
            bool reload;
            bool enabled {false};
            bool negate {false};
            bool trueNegate {true};
            u8_fast shiftCount {0};
            u16_fast period {0};

            Counter<s8_fast> timer{0, [&] () {
                if (
                        enabled 
                     && shiftCount > 0 
                     && period >= 8 
                     && targetPeriod() <= 0x7FF) {
                    period = targetPeriod();
                }
                reload = false;
                
                return true;
            }};
                     
            u16_fast targetPeriod() const {
                s16_fast change = period >> shiftCount;
                change = (negate ? -change : change);
                change -= (period + change >= 1) && negate && trueNegate; 
                return period + change;
            }

            void tick() {
                timer.tick();
                if (reload) {
                    reload = false;
                    timer.counter = timer.reload;
                }
            }
        };
        struct Envelope {
            bool start {true};
            bool loop {false};

            Counter<s8_fast> decayLevel{15, [&] () {
                if (!loop) {
                    decayLevel.counter = 0;
                }
                
                return true;
            }};
            Counter<s8_fast> timer{0, [&] () {
                decayLevel.tick();
                return true;
            }};

            void tick() {
                if (start) {
                    start = false;
                    decayLevel.counter = decayLevel.reload;
                    timer.counter = timer.reload;
                }
                else {
                    timer.tick();
                }
            }

            u8_fast output() const {
                return decayLevel.counter;
            }
        };

        //Sound channels:
        struct Pulse {
            const std::array<std::array<bool, 8>, 4> sequences {{
                {0, 0, 0, 0, 0, 0, 0, 1},
                {0, 0, 0, 0, 0, 0, 1, 1},
                {0, 0, 0, 0, 1, 1, 1, 1},
                {1, 1, 1, 1, 1, 1, 0, 0},
            }};

            u8_fast duty {0};
            bool ignoreEnvelope {false};

            LengthCounter lengthCounter;
            Envelope envelope;
            Sweep sweep;
            
            Counter<s8_fast> sequencePos{7, [] () {}};
            Counter<s16_fast> timer{0, [&] () {
                sequencePos.tick();
                //Override default reload logic:
                timer.counter = sweep.period * 2 + 1;
            }};

            void tick() {
                timer.tick();
            }

            u8_fast output() const {
                if (
                        sequences[duty][sequencePos.counter] 
                     && sweep.targetPeriod() <= 0x7FF
                     && lengthCounter.counter.counter > 0
                     && sweep.period >= 8) {
                    return ignoreEnvelope 
                          ? envelope.timer.reload 
                          : envelope.output();
                }
                else {
                    return 0; 
                }
            } 
        };
        struct Triangle {
            bool ascending; 
            u8_fast volume;

            LinearCounter linearCounter;
            LengthCounter lengthCounter;

            Counter<s16_fast> timer{0, [&] () {
                if (
                        lengthCounter.counter.counter > 0 
                     && linearCounter.isNonZero()) {
                    if (ascending) {
                        if (volume == 15) {
                            ascending = false;
                        }
                        else {
                            ++volume;
                        }
                    }    
                    else {
                        if (volume == 0) {
                            ascending = true;
                        }
                        else {
                            --volume;
                        }
                    }
                }
            }};

            void tick() {
                timer.tick();
            }

            u8_fast output() const {
                return volume;
            }
        };
        struct Noise {
            bool mode {false};
            bool ignoreEnvelope {false};
            u16_fast lfsr {0x0001};

            Envelope envelope;
            LengthCounter lengthCounter;

            Counter<s16_fast> timer{3, [&] () {
                bool feedback = (lfsr ^ (mode ? lfsr >> 6 : lfsr >> 1)) & 1;
                lfsr >>= 1;
                setBit(lfsr, 14, feedback);
            }};

            void tick() {
                timer.tick();
            }

            u8_fast output() const {
                if (lengthCounter.counter.counter > 0 && lfsr & 1) {
                    return ignoreEnvelope 
                          ? envelope.timer.reload 
                          : envelope.output();
                }
                else {
                    return 0;
                }
            }
        };
        struct Dmc {
            Apu& apu;

            u8_fast irqId;
            u8_fast volume {0};
            bool irqEnabled {false};
            bool silence {true};
            bool enabled {false};
            bool finished {true}; 
            bool loop {false};
            u8_fast shiftRegister;
            //-1 indicates an empty sample buffer:
            s16_fast sampleBuffer {0};
            u16_fast startAddress {0xC000};
            u16_fast address;

            Dmc(Apu& apu)
                  : apu{apu} {
                irqId = apu.cpu.connectIrq();
            }

            void fillSampleBuffer() {
                if (sampleBuffer == -1 && !finished) {
                    //TODO: proper cpu stall 
                    apu.cpu.timer.counter += 4 * (apu.cpu.timer.reload + 1); 
                    sampleBuffer = apu.cpu.memory[address++];
                    address |= 0x8000;
                    bytesRemaining.tick();
                }
            }

            Counter<s16_fast> bytesRemaining{0, [&] () {
                if (loop) {
                    address = startAddress;
                }
                else {
                    finished = true;
                    bytesRemaining.counter = 0;
                    if (irqEnabled) {
                        apu.cpu.pullIrq(irqId);
                    }
                }
            }};

            Counter<s8_fast> bitsRemaining{7, [&] () {
                if (sampleBuffer == -1) {
                    silence = true;
                }
                else {
                    silence = false;
                    shiftRegister = sampleBuffer;
                    sampleBuffer = -1;
                }
            }};
            Counter<s16_fast> timer{427, [&] () {
                if (!silence) {
                    if (shiftRegister & 0x01 && volume <= 125) {
                        volume += 2;
                    }
                    else if (!(shiftRegister & 0x01) && volume >= 2) {
                        volume -= 2;
                    }
                }
                shiftRegister >>= 1;
                bitsRemaining.tick();
            }};

            void tick() {
                fillSampleBuffer();
                timer.tick();
            }

            void toggle(const bool enable) {
                enabled = enable;
                if (enabled && finished) {
                    address = startAddress;
                    bytesRemaining.counter = bytesRemaining.reload;
                }
                else if (!enabled) { 
                    bytesRemaining.counter = 0;
                }
                finished = !enabled; 
            }

            u8_fast output() const {
                return volume;
            }
        };

        //Frame counter:
        struct FrameCounter {
            Apu& apu;

            u32_fast cycle = -3;
            bool fourStep {true}; 
            bool interruptInhibit {false}; 
            u8_fast irqId;

            FrameCounter(Apu& apu)
                  : apu{apu} {
                irqId = apu.cpu.connectIrq();
            }

            void tick() {
                ++cycle;
                if (
                        cycle == 7457 
                     || cycle == 14913
                     || cycle == 22371
                     || (fourStep && cycle == 29829)
                     || (!fourStep && cycle == 37281)) {
                    apu.pulse1.envelope.tick();
                    apu.pulse2.envelope.tick();
                    apu.triangle.linearCounter.tick();
                    apu.noise.envelope.tick();
                }
                if (
                        cycle == 14913
                     || (fourStep && cycle == 29829)
                     || (!fourStep && cycle == 37281)) {
                    apu.pulse1.sweep.tick();
                    apu.pulse2.sweep.tick();

                    apu.pulse1.lengthCounter.tick();
                    apu.pulse2.lengthCounter.tick();
                    apu.triangle.lengthCounter.tick();
                    apu.noise.lengthCounter.tick();
                }
                if (fourStep && cycle >= 29828 && cycle <= 29830) {
                    apu.cpu.pullIrq(irqId);
                }

                if (
                        (fourStep && cycle == 29830)
                     || (!fourStep && cycle == 37282)) {
                    cycle = 0;
                }

                if (interruptInhibit) {
                    apu.cpu.releaseIrq(irqId);
                }

            } 
        };

        Cpu& cpu;

        //Tick counter:
        u32_fast cycle {0};

        FrameCounter frameCounter{*this};

        //Channels:
        Pulse pulse1; 
        Pulse pulse2;
        Triangle triangle;
        Noise noise;
        Dmc dmc{*this};

        //DAC output approximation tables:
        const std::array<double, 31> pulseOutput {
            0,
            0.01160913952357, 0.02293948126801, 0.03400094921689, 
            0.04480300187617, 0.05535465924895, 0.06566452795600, 
            0.07574082464884, 0.08559139784946, 0.09522374833850, 
            0.10464504820333, 0.11386215864759, 0.12288164665523, 
            0.13170980059398, 0.14035264483627, 0.14881595346905, 
            0.15710526315789, 0.16522588522589, 0.17318291700242,
            0.18098125249302, 0.18862559241706, 0.19612045365663, 
            0.20347017815647, 0.21067894131185, 0.21775075987842, 
            0.22468949943545, 0.23149888143177, 0.23818248984115, 
            0.24474377745242, 0.25118607181719, 0.25751258087707, 
        };
        const std::array<double, 203> tndOutput {
            0,
            0.00669982397969, 0.01334502018019, 0.01993625400950, 
            0.02647418011241, 0.03295944258729, 0.03939267519756, 
            0.04577450157816, 0.05210553543714, 0.05838638075230, 
            0.06461763196336, 0.07079987415942, 0.07693368326217, 
            0.08301962620469, 0.08905826110614, 0.09505013744241, 
            0.10099579621273, 0.10689577010258, 0.11275058364270, 
            0.11856075336460, 0.12432678795245, 0.13004918839154, 
            0.13572844811339, 0.14136505313756, 0.14695948221033, 
            0.15251220694025, 0.15802369193063, 0.16349439490917, 
            0.16892476685466, 0.17431525212090, 0.17966628855794, 
            0.18497830763061, 0.19025173453449, 0.19548698830939, 
            0.20068448195030, 0.20584462251608, 0.21096781123563, 
            0.21605444361197, 0.22110490952398, 0.22611959332601, 
            0.23109887394543, 0.23604312497802, 0.24095271478145, 
            0.24582800656677, 0.25066935848794, 0.25547712372958, 
            0.26025165059283, 0.26499328257949, 0.26970235847437, 
            0.27437921242602, 0.27902417402571, 0.28363756838493, 
            0.28821971621118, 0.29277093388234, 0.29729153351946, 
            0.30178182305810, 0.30624210631829, 0.31067268307303, 
            0.31507384911547, 0.31944589632472, 0.32378911273039, 
            0.32810378257583, 0.33239018638016, 0.33664860099905, 
            0.34087929968434, 0.34508255214246, 0.34925862459181, 
            0.35340777981888, 0.35753027723345, 0.36162637292260, 
            0.36569631970379, 0.36974036717681, 0.37375876177487, 
            0.37775174681463, 0.38171956254531, 0.38566244619686, 
            0.38958063202731, 0.39347435136907, 0.39734383267453, 
            0.40118930156071, 0.40501098085310, 0.40880909062876, 
            0.41258384825848, 0.41633546844831, 0.42006416328027, 
            0.42377014225228, 0.42745361231741, 0.43111477792243, 
            0.43475384104561, 0.43837100123386, 0.44196645563923, 
            0.44554039905471, 0.44909302394942, 0.45262452050314, 
            0.45613507664028, 0.45962487806320, 0.46309410828495, 
            0.46654294866144, 0.46997157842304, 0.47338017470566, 
            0.47676891258120, 0.48013796508757, 0.48348750325813, 
            0.48681769615063, 0.49012871087564, 0.49342071262454, 
            0.49669386469696, 0.49994832852779, 0.50318426371374, 
            0.50640182803940, 0.50960117750290, 0.51278246634113, 
            0.51594584705452, 0.51909147043139, 0.52221948557194, 
            0.52533003991180, 0.52842327924518, 0.53149934774765, 
            0.53455838799856, 0.53760054100306, 0.54062594621377, 
            0.54363474155206, 0.54662706342906, 0.54960304676622, 
            0.55256282501566, 0.55550653018002, 0.55843429283219, 
            0.56134624213454, 0.56424250585795, 0.56712321040049, 
            0.56998848080581, 0.57283844078121, 0.57567321271550, 
            0.57849291769646, 0.58129767552811, 0.58408760474768, 
            0.58686282264231, 0.58962344526546, 0.59236958745312, 
            0.59510136283973, 0.59781888387383, 0.60052226183351, 
            0.60321160684160, 0.60588702788061, 0.60854863280746, 
            0.61119652836797, 0.61383082021115, 0.61645161290323, 
            0.61905900994148, 0.62165311376788, 0.62423402578250, 
            0.62680184635674, 0.62935667484632, 0.63189860960408, 
            0.63442774799265, 0.63694418639685, 0.63944802023592, 
            0.64193934397562, 0.64441825114007, 0.64688483432350, 
            0.64933918520172, 0.65178139454352, 0.65421155222187, 
            0.65662974722489, 0.65903606766677, 0.66143060079845, 
            0.66381343301815, 0.66618464988179, 0.66854433611320, 
            0.67089257561425, 0.67322945147475, 0.67555504598228, 
            0.67786944063185, 0.68017271613539, 0.68246495243116, 
            0.68474622869301, 0.68701662333945, 0.68927621404268, 
            0.69152507773743, 0.69376329062966, 0.69599092820525, 
            0.69820806523840, 0.70041477580004, 0.70261113326609, 
            0.70479721032554, 0.70697307898854, 0.70913881059424, 
            0.71129447581863, 0.71344014468223, 0.71557588655763, 
            0.71770177017702, 0.71981786363950, 0.72192423441843, 
            0.72402094936854, 0.72610807473301, 0.72818567615049, 
            0.73025381866193, 0.73231256671739, 0.73436198418274, 
            0.73640213434624, 0.73843307992511, 0.74045488307187, 
            0.74246760538076,
        };

        //Miscellaneous lookup tables:
        const std::array<u16_fast, 16> noisePeriods {
               3,    7,   15,   31,   63,   95,  127,  159, 
             201,  253,  379,  507,  761, 1015, 2033, 4067,
        };
        const std::array<u8_fast, 32> lengths {
             10, 254,  20,   2,  40,   4,  80,   6,
            160,   8,  60,  10,  14,  12,  26,  14,
             12,  16,  24,  18,  48,  20,  96,  22,
            192,  24,  72,  26,  16,  28,  32,  30,
        };
        const std::array<u16_fast, 16> dmcPeriods {
            427, 379, 339, 319, 285, 253, 225, 213, 
            189, 159, 141, 127, 105,  83,  71,  53,
        };

        //$4000 - $4003 register write handlers:
        void pulseWrite0(Pulse& pulse, u8 data) {
            pulse.envelope.timer.reload = data & 0x0F;
            pulse.ignoreEnvelope = data & 0x10;
            pulse.envelope.loop =
                    pulse.lengthCounter.halt = data & 0x20;
            pulse.duty = data >> 6;
        };
        void pulseWrite1(Pulse& pulse, u8 data) {
            pulse.sweep.shiftCount = data & 0x07;
            pulse.sweep.negate = data & 0x08;
            pulse.sweep.timer.reload = (data >> 4) & 0x07;
            pulse.sweep.enabled = data & 0x80;
            pulse.sweep.reload = true;
        };
        void pulseWrite2(Pulse& pulse, u8 data) const {
            pulse.sweep.period &= 0x0700;
            pulse.sweep.period |= data;
        };
        void pulseWrite3(Pulse& pulse, u8 data) const {
            pulse.sweep.period &= 0x00FF;
            pulse.sweep.period |= (data & 0x07) << 8;
            if (pulse.lengthCounter.enabled) {
                pulse.lengthCounter.counter.counter = lengths[data >> 3];
            }
            pulse.sequencePos.counter = 0;
            pulse.envelope.start = true;
        }

    public:
        //Function that outputs samples to the audio device:
        std::function<void(u8 sample)> outputFunction {[] (u8) {}};

        Apu(Cpu& cpu) 
              : cpu{cpu} { 
            pulse2.sweep.trueNegate = false;

            //Unused addresses:
            for (u16 address : {0x4009, 0x400D}) {
                cpu.memory.writeFunctions[address] = [] (
                        MappedMemory<>* const memory,
                        const u16 address,
                        const u8 data) {
                    //TODO: APU test mode / open bus
                };
                cpu.memory.readFunctions[address] = [] (
                        MappedMemory<>* const memory,
                        const u16 address) {
                    //TODO: APU test mode / open bus
                    return 0;
                };
            }
            
            //APU register mapping:
            cpu.memory.writeFunctions[0x4000] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                pulseWrite0(pulse1, data);
            };
            cpu.memory.writeFunctions[0x4001] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                pulseWrite1(pulse1, data);
            };
            cpu.memory.writeFunctions[0x4002] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                pulseWrite2(pulse1, data);
            };
            cpu.memory.writeFunctions[0x4003] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                pulseWrite3(pulse1, data);
            };
            cpu.memory.writeFunctions[0x4004] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                pulseWrite0(pulse2, data);
            };
            cpu.memory.writeFunctions[0x4005] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                pulseWrite1(pulse2, data);
            };
            cpu.memory.writeFunctions[0x4006] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                pulseWrite2(pulse2, data);
            };
            cpu.memory.writeFunctions[0x4007] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                pulseWrite3(pulse2, data);
            };
            cpu.memory.writeFunctions[0x4008] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                triangle.linearCounter.counter.reload = data & 0x7F;
                triangle.lengthCounter.halt 
                      = triangle.linearCounter.control = data & 0x80;
            };
            cpu.memory.writeFunctions[0x400A] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                triangle.timer.reload &= 0x0700;
                triangle.timer.reload |= data;
            };
            cpu.memory.writeFunctions[0x400B] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                triangle.timer.reload &= 0x00FF;
                triangle.timer.reload |= (data & 0x07) << 8;
                if (triangle.lengthCounter.enabled) {
                    triangle.lengthCounter.counter.counter = lengths[data >> 3];
                }
                triangle.linearCounter.reload = true;
            };
            cpu.memory.writeFunctions[0x400C] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                noise.envelope.timer.reload = data & 0x0F;
                noise.ignoreEnvelope = data & 0x10;
                noise.envelope.loop
                      = noise.lengthCounter.halt = data & 0x20;
            };
            cpu.memory.writeFunctions[0x400E] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                noise.timer.reload = noisePeriods[data & 0x0F];
                noise.mode = data & 0x80;
            };
            cpu.memory.writeFunctions[0x400F] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                if (noise.lengthCounter.enabled) {
                    noise.lengthCounter.counter.counter = lengths[data >> 3];
                }
                noise.envelope.start = true;
            };
            cpu.memory.writeFunctions[0x4010] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                dmc.timer.reload = dmcPeriods[data & 0x0F];
                dmc.loop = data & 0x40;
                dmc.irqEnabled = data & 0x80;
                if (!dmc.irqEnabled) {
                    cpu.releaseIrq(dmc.irqId);
                }
            };
            cpu.memory.writeFunctions[0x4011] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                //DISCREPANCY: This operation occasionally fails 
                //on console, not going to emulate though
                dmc.volume = data & 0x7F;
            };
            cpu.memory.writeFunctions[0x4012] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                dmc.startAddress = 0xC000 + (data << 6);
            };
            cpu.memory.writeFunctions[0x4013] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                dmc.bytesRemaining.reload = data << 4; 
            };
            cpu.memory.writeFunctions[0x4015] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                pulse1.lengthCounter.toggle(data & 0x01);
                pulse2.lengthCounter.toggle(data & 0x02);
                triangle.lengthCounter.toggle(data & 0x04);
                noise.lengthCounter.toggle(data & 0x08);
                dmc.toggle(data & 0x10);
                
                cpu.releaseIrq(dmc.irqId);
            };
            cpu.memory.readFunctions[0x4015] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address) {
                u8 data;
                setBit(data, 0, pulse1.lengthCounter.counter.counter > 0);
                setBit(data, 1, pulse2.lengthCounter.counter.counter > 0);
                setBit(data, 2, triangle.lengthCounter.counter.counter > 0);
                setBit(data, 3, noise.lengthCounter.counter.counter > 0);
                setBit(data, 4, !dmc.finished); 
                //TODO: Open bus bit 5
                setBit(data, 6, cpu.isPullingIrq(frameCounter.irqId));
                setBit(data, 7, cpu.isPullingIrq(dmc.irqId));
                
                //TODO: Interrupt race condition where bits 6 and 7
                //of data are set but IRQ is not released if the read 
                //occurs on the same cycle the interrupt is generated:
                cpu.releaseIrq(frameCounter.irqId);

                return data;
            };
            cpu.memory.writeFunctions[0x4017] = [&] (
                    MappedMemory<>* const memory,
                    const u16 address,
                    const u8 data) {
                memory->memory[0x4017] = data;

                frameCounter.interruptInhibit = data & 0x40;
                frameCounter.fourStep = !(data & 0x80);

                frameCounter.cycle = -3;
                frameCounter.cycle -= cycle & 0x01;

                if (!frameCounter.fourStep) {
                    pulse1.envelope.tick();
                    pulse2.envelope.tick();
                    triangle.linearCounter.tick();
                    noise.envelope.tick();

                    pulse1.sweep.tick();
                    pulse2.sweep.tick();

                    pulse1.lengthCounter.tick();
                    pulse2.lengthCounter.tick();
                    triangle.lengthCounter.tick();
                    noise.lengthCounter.tick();
                }
            };
        }

        Counter<s8_fast> output{29, [&] () {
            outputFunction((pulseOutput[
                           pulse1.output()
                         + pulse2.output()]
                     + tndOutput[
                            3 * triangle.output()
                          + 2 * noise.output()
                          + dmc.output()]) * 0x100);
        }};

        Counter<s16_fast> timer{0, [&] () {
            pulse1.tick();
            pulse2.tick();
            triangle.tick();
            noise.tick();
            dmc.tick();
            frameCounter.tick();
            output.tick();

            ++cycle;
        }};

        void reset() {
            cpu.memory[0x4015] = 0;
            cpu.memory[0x4017] = cpu.memory.memory[0x4017];

            triangle.volume = 15;
            triangle.ascending = false;

            dmc.volume &= 0x01;
        }

        void tick(const u8_fast ticks = 1) {
            timer.tick(ticks);
        }

        template <typename StateType>
        void dumpState(StateType& state) {
            //TODO: finish dump and load state methods
            auto dump {[&] (const u8 data) {
                //                  size in bytes
                state.write(&data,              1);
            }};

            dump(cycle & 0x000000FF);
            dump(cycle >> 8 & 0x0000FF);
            dump(cycle >> 16 & 0x00FF);
            dump(cycle >> 24);

            auto dumpPulse {[&] (const Pulse& pulse) {
                dump(pulse.duty);
                dump(pulse.ignoreEnvelope);
                //Length counter:
                    dump(pulse.lengthCounter.enabled);
                    dump(pulse.lengthCounter.halt);
                    dump(pulse.lengthCounter.counter.reload & 0x00FF);
                    dump(pulse.lengthCounter.counter.reload >> 8);
                    dump(pulse.lengthCounter.counter.counter & 0x00FF);
                    dump(pulse.lengthCounter.counter.counter >> 8);
                //Envelope:
                    dump(pulse.envelope.start);
                    dump(pulse.envelope.loop);
                    dump(pulse.envelope.decayLevel.reload);
                    dump(pulse.envelope.decayLevel.counter);
                    dump(pulse.envelope.timer.reload);
                    dump(pulse.envelope.timer.counter);
                //Sweep:
                    dump(pulse.sweep.reload);
                    dump(pulse.sweep.enabled);
                    dump(pulse.sweep.negate);
                    dump(pulse.sweep.trueNegate);
                    dump(pulse.sweep.shiftCount);
                    dump(pulse.sweep.period & 0x00FF);
                    dump(pulse.sweep.period >> 8);
                    dump(pulse.sweep.timer.reload);
                    dump(pulse.sweep.timer.counter);
                dump(pulse.sequencePos.reload);
                dump(pulse.sequencePos.counter);
                dump(pulse.timer.reload & 0x00FF);
                dump(pulse.timer.reload >> 8);
                dump(pulse.timer.counter & 0x00FF);
                dump(pulse.timer.counter >> 8);
            }};
            dumpPulse(pulse1);
            dumpPulse(pulse2);
            
            dump(triangle.ascending);
            dump(triangle.volume);
            //Linear counter:
                dump(triangle.linearCounter.reload);
                dump(triangle.linearCounter.control);
                dump(triangle.linearCounter.counter.reload & 0x00FF);
                dump(triangle.linearCounter.counter.reload >> 8);
                dump(triangle.linearCounter.counter.counter & 0x00FF);
                dump(triangle.linearCounter.counter.counter >> 8);
            //Length counter:
                dump(triangle.lengthCounter.enabled);
                dump(triangle.lengthCounter.halt);
                dump(triangle.lengthCounter.counter.reload & 0x00FF);
                dump(triangle.lengthCounter.counter.reload >> 8);
                dump(triangle.lengthCounter.counter.counter & 0x00FF);
                dump(triangle.lengthCounter.counter.counter >> 8);
            dump(triangle.timer.reload & 0x00FF);
            dump(triangle.timer.reload >> 8);
            dump(triangle.timer.counter & 0x00FF);
            dump(triangle.timer.counter >> 8);

            dump(noise.mode);
            dump(noise.ignoreEnvelope);
            dump(noise.lfsr & 0x00FF);
            dump(noise.lfsr >> 8);
            //Length counter:
                dump(noise.lengthCounter.enabled);
                dump(noise.lengthCounter.halt);
                dump(noise.lengthCounter.counter.reload & 0x00FF);
                dump(noise.lengthCounter.counter.reload >> 8);
                dump(noise.lengthCounter.counter.counter & 0x00FF);
                dump(noise.lengthCounter.counter.counter >> 8);
            //Envelope:
                dump(noise.envelope.start);
                dump(noise.envelope.loop);
                dump(noise.envelope.decayLevel.reload);
                dump(noise.envelope.decayLevel.counter);
                dump(noise.envelope.timer.reload);
                dump(noise.envelope.timer.counter);
            dump(noise.timer.reload & 0x00FF);
            dump(noise.timer.reload >> 8);
            dump(noise.timer.counter & 0x00FF);
            dump(noise.timer.counter >> 8);

            dump(dmc.irqId);
            dump(dmc.volume);
            dump(dmc.irqEnabled);
            dump(dmc.silence);
            dump(dmc.enabled);
            dump(dmc.finished);
            dump(dmc.loop);
            dump(dmc.shiftRegister);
            dump(dmc.sampleBuffer & 0x00FF);
            dump(dmc.sampleBuffer >> 8);
            dump(dmc.startAddress & 0x00FF);
            dump(dmc.startAddress >> 8);
            dump(dmc.address & 0x00FF);
            dump(dmc.address >> 8);

            dump(frameCounter.cycle & 0x000000FF);
            dump(frameCounter.cycle >> 8 & 0x0000FF);
            dump(frameCounter.cycle >> 16 & 0x00FF);
            dump(frameCounter.cycle >> 24);
            dump(frameCounter.fourStep);
            dump(frameCounter.interruptInhibit);
            dump(frameCounter.irqId);
        }

        template <typename StateType>
        void loadState(StateType& state) {
            u16 tmp;
            auto load {[&] () {
                u8 result;
                //                  size in bytes 
                state.read(&result,             1);
                return result;
            }};

            cycle = load();
            cycle |= load() << 8;
            cycle |= load() << 16;
            cycle |= load() << 24;

            auto loadPulse {[&] (Pulse& pulse) {
                pulse.duty = load();
                pulse.ignoreEnvelope = load();
                //Length counter:
                    pulse.lengthCounter.enabled = load();
                    pulse.lengthCounter.halt = load();
                    tmp = load();
                    tmp |= load() << 8;
                    pulse.lengthCounter.counter.reload = toSigned(tmp); 
                    tmp = load();
                    tmp |= load() << 8;
                    pulse.lengthCounter.counter.counter = toSigned(tmp);
                //Envelope:
                    pulse.envelope.start = load();
                    pulse.envelope.loop = load();
                    pulse.envelope.decayLevel.reload = toSigned(load());
                    pulse.envelope.decayLevel.counter = toSigned(load());
                    pulse.envelope.timer.reload = toSigned(load());
                    pulse.envelope.timer.counter = toSigned(load());
                //Sweep:
                    pulse.sweep.reload = load();
                    pulse.sweep.enabled = load();
                    pulse.sweep.negate = load();
                    pulse.sweep.trueNegate = load();
                    pulse.sweep.shiftCount = load();
                    pulse.sweep.period = load();
                    pulse.sweep.period |= load() << 8;
                    pulse.sweep.timer.reload = toSigned(load());
                    pulse.sweep.timer.counter = toSigned(load());
                pulse.sequencePos.reload = toSigned(load());
                pulse.sequencePos.counter = toSigned(load());
                tmp = load();
                tmp |= load() << 8;
                pulse.timer.reload = toSigned(tmp);
                tmp = load();
                tmp |= load() << 8;
                pulse.timer.counter = toSigned(tmp);
            }};
            loadPulse(pulse1);
            loadPulse(pulse2);

            triangle.ascending = load();
            triangle.volume = load();
            //Linear counter:
                triangle.linearCounter.reload = load(); 
                triangle.linearCounter.control = load();
                tmp = load();
                tmp |= load() << 8;
                triangle.linearCounter.counter.reload = toSigned(tmp);
                tmp = load();
                tmp |= load() << 8;
                triangle.linearCounter.counter.counter = toSigned(tmp);
            //Length counter:
                triangle.lengthCounter.enabled = load();
                triangle.lengthCounter.halt = load();
                tmp = load();
                tmp |= load() << 8;
                triangle.lengthCounter.counter.reload = toSigned(tmp); 
                tmp = load();
                tmp |= load() << 8;
                triangle.lengthCounter.counter.counter = toSigned(tmp);
            tmp = load();
            tmp |= load() << 8;
            triangle.timer.reload = toSigned(tmp);
            tmp = load();
            tmp |= load() << 8;
            triangle.timer.counter = toSigned(tmp);

            noise.mode = load();
            noise.ignoreEnvelope = load();
            noise.lfsr = load();
            noise.lfsr |= load() << 8;
            //Length counter:
                noise.lengthCounter.enabled = load();
                noise.lengthCounter.halt = load();
                tmp = load();
                tmp |= load() << 8;
                noise.lengthCounter.counter.reload = toSigned(tmp); 
                tmp = load();
                tmp |= load() << 8;
                noise.lengthCounter.counter.counter = toSigned(tmp);
            //Envelope:
                noise.envelope.start = load();
                noise.envelope.loop = load();
                noise.envelope.decayLevel.reload = toSigned(load());
                noise.envelope.decayLevel.counter = toSigned(load());
                noise.envelope.timer.reload = toSigned(load());
                noise.envelope.timer.counter = toSigned(load());
            tmp = load();
            tmp |= load() << 8;
            noise.timer.reload = toSigned(tmp);
            tmp = load();
            tmp |= load() << 8;
            noise.timer.counter = toSigned(tmp);

            dmc.irqId = load();
            dmc.volume = load();
            dmc.irqEnabled = load();
            dmc.silence = load();
            dmc.enabled = load();
            dmc.finished = load();
            dmc.loop = load();
            dmc.shiftRegister = load();
            tmp = load();
            tmp |= load() << 8;
            dmc.sampleBuffer = toSigned(tmp);
            dmc.startAddress = load();
            dmc.startAddress |= load() << 8;
            dmc.address = load();
            dmc.address |= load() << 8;

            frameCounter.cycle = load();
            frameCounter.cycle |= load() << 8;
            frameCounter.cycle |= load() << 16;
            frameCounter.cycle |= load() << 24;
            frameCounter.fourStep = load();
            frameCounter.interruptInhibit = load();
            frameCounter.irqId = load();
        }
};

