#pragma once
#ifdef OS_ANDROID
    #include <cstdint>
    #define STDINT_NAMESPACE std
#else
    #include <tr1/cstdint>
    #define STDINT_NAMESPACE std::tr1
#endif
using u8 =  STDINT_NAMESPACE::uint8_t;
using u16 = STDINT_NAMESPACE::uint16_t;
using u32 = STDINT_NAMESPACE::uint32_t;
using u64 = STDINT_NAMESPACE::uint64_t;

using s8 =  STDINT_NAMESPACE::int8_t;
using s16 = STDINT_NAMESPACE::int16_t;
using s32 = STDINT_NAMESPACE::int32_t;
using s64 = STDINT_NAMESPACE::int64_t;

using u8_fast  = STDINT_NAMESPACE::uint_fast8_t;
using u16_fast = STDINT_NAMESPACE::uint_fast16_t;
using u32_fast = STDINT_NAMESPACE::uint_fast32_t;
using u64_fast = STDINT_NAMESPACE::uint_fast64_t;

using s8_fast  = STDINT_NAMESPACE::int_fast8_t;
using s16_fast = STDINT_NAMESPACE::int_fast16_t;
using s32_fast = STDINT_NAMESPACE::int_fast32_t;
using s64_fast = STDINT_NAMESPACE::int_fast64_t;
#undef STDINT_NAMESPACE

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
