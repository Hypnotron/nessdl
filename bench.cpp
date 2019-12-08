#include "byte.hpp"

int main() {
    u8 data[] {0x01, 0x23, 0x45, 0x67};
    for (u64 i {0}; i < 1000000LL; ++i) {
        std::cout << readBytes<4, s32>(&data[0]);
        std::cout << readBytes<4, u32>(&data[0]);
        std::cout << readBytes<2, s16>(&data[0]);
        std::cout << readBytes<2, u16>(&data[0]);
        std::cout << readBytes<1, s8>(&data[0]);
        std::cout << readBytes<1, u8>(&data[0]);
    }

    return 0;
}
