#pragma once
#include <tr1/cstdint>

using u8 =  std::tr1::uint8_t;
using u16 = std::tr1::uint16_t;
using u32 = std::tr1::uint32_t;
using u64 = std::tr1::uint64_t;

using s8 =  std::tr1::int8_t;
using s16 = std::tr1::int16_t;
using s32 = std::tr1::int32_t;
using s64 = std::tr1::int64_t;

using u8_fast  = std::tr1::uint_fast8_t;
using u16_fast = std::tr1::uint_fast16_t;
using u32_fast = std::tr1::uint_fast32_t;
using u64_fast = std::tr1::uint_fast64_t;

using s8_fast  = std::tr1::int_fast8_t;
using s16_fast = std::tr1::int_fast16_t;
using s32_fast = std::tr1::int_fast32_t;
using s64_fast = std::tr1::int_fast64_t;

enum class Endianness {
    LITTLE,
    BIG
};

template <
        u8 byteCount, 
        typename DataType, 
        Endianness endianness = Endianness::LITTLE,
        typename AddressType>
DataType readBytes(AddressType address) { 
    constexpr bool little {endianness == Endianness::LITTLE};
    DataType data {0};
    for (
            s16 shiftCount {little ? 0 : (byteCount - 1) * 8};
            little ? shiftCount < (byteCount * 8) : shiftCount >= 0;
            shiftCount += (little ? 8 : -8), ++address) {
        data |= *address << shiftCount; 
    }
    return data;
}

template <
        u8 byteCount, 
        Endianness endianness = Endianness::LITTLE, 
        typename DataType, 
        typename AddressType>
void writeBytes(AddressType address, DataType data) {
    constexpr bool little {endianness == Endianness::LITTLE};
    for (u8 byteIndex {0}; byteIndex < byteCount; ++byteIndex, ++address) {
        *address = little ? data : data >> ((byteCount - 1) * 8); 
        data = little ? data >> 8 : data << 8;
    }
}
