#pragma once
#include <cstdint>
using u8 =  std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using s8 =  std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;

using u8_fast  = std::uint_fast8_t;
using u16_fast = std::uint_fast16_t;
using u32_fast = std::uint_fast32_t;
using u64_fast = std::uint_fast64_t;

using s8_fast  = std::int_fast8_t;
using s16_fast = std::int_fast16_t;
using s32_fast = std::int_fast32_t;
using s64_fast = std::int_fast64_t;

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

s8 toSigned(u8 value) {
    if (value <= INT8_MAX) {
        return value;
    }
    else {
        return static_cast<s8>(value - INT8_MIN) + INT8_MIN;
    }
}
s16 toSigned(u16 value) {
    if (value <= INT16_MAX) {
        return value;
    }
    else {
        return static_cast<s16>(value - INT16_MIN) + INT16_MIN;
    }
}
s32 toSigned(u32 value) {
    if (value <= INT32_MAX) {
        return value;
    }
    else {
        return static_cast<s32>(value - INT32_MIN) + INT32_MIN;
    }
}
s64 toSigned(u64 value) {
    if (value <= INT64_MAX) {
        return value;
    }
    else {
        return static_cast<s64>(value - INT64_MIN) + INT64_MIN;
    }
}

template <typename DataType>
void setBit(DataType data, u8_fast bit, bool value) {
    data &= ~(1 << bit);
    data |= value << bit;
}

