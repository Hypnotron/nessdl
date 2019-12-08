#include <iostream>
#include <array>
#include <map>
#include <functional>
#include "byte.hpp"
#include "memory.hpp"
#include <vector>

struct Shoal {
    u16 memberData {505};
    std::function<void(Shoal*, u16)> memberDemo;
    void callMemberDemo(u16 arg) {
        memberDemo(this, arg);
    }
};

struct PPU {
    u16 demoData {800};
    void ppuctrlWrite() const {
        std::cout << "Dummy " << demoData << "\n";
    }
};

int main() {
    std::map<u16, void (*)(u16)> demo;
    demo[0xABCD] = [](u16 arg) {
        std::cout << "LAMBDA " << std::hex << arg << std::dec << "\n";
    };
    demo.lower_bound(0xABCD)->second(0x2016);
    
    Shoal shoal;
    PPU hobart;
    shoal.memberData = 666;
    shoal.memberDemo = [&hobart] (Shoal* shoal, u16 arg) {
        hobart.ppuctrlWrite();
        std::cout << shoal->memberData << " " << arg << "\n";
    };
    hobart.demoData = 1010;
    shoal.memberData = 777;
    shoal.callMemberDemo(420);

    MappedMemory<> shoal2(0xF000);
    shoal2.memory[0x400] = 0x61;
    shoal2.readFunctions[0x500] = [] (MappedMemory<>* const mappedMemory, u16 address) {
        std::cout << "READING\n\nmake -B\n";
        return mappedMemory->memory[address + 0xB0];
    };
    shoal2.writeFunctions[0x400] = [] (MappedMemory<>* const mappedMemory, u16 address, u8 data) {
        std::cout << "WRITING\n\nhailm*ry\n";
        mappedMemory->memory[address + 0xC0] = data;
    };
    shoal2[0x400] = 0x67;
    std::cout << shoal2[0x410];
    
    std::vector<u16> robThomas {656};
    std::cout << "IMP " << *(robThomas.end() - 1);
    shoal2.begin();

    u16 shoal3 = 0xFFFF;
    shoal3 = 1 == 1 ? shoal3 >> 4 : shoal3 << 4;
    std::cout << "HEX SAVE ME " << std::hex << shoal3 << "\n";

    u8 datt[4] {0x01, 0x23, 0x45, 0x67}; 
    std::cout << readBytes<4, s32, Endianness::BIG>(&datt[0]);

    return 0;
}
