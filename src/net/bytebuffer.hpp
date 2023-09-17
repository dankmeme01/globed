#pragma once

#include <iostream>
#include <vector>
#include <cstring>
#include <bit>

constexpr bool LITTLE_ENDIAN = std::endian::native == std::endian::little;

inline uint16_t byteswapU16(uint16_t val) {
    #ifdef _MSC_VER
    return _byteswap_ushort(val);
    #else
    return (val >> 8) | (val << 8);
    #endif
}

inline uint32_t byteswapU32(uint32_t val) {
    #ifdef _MSC_VER
    return _byteswap_ulong(val);
    #else
    return __builtin_bswap32(val);
    #endif
}

inline uint64_t byteswapU64(uint64_t val) {
    #ifdef _MSC_VER
    return _byteswap_uint64(val);
    #else
    return __builtin_bswap64(val);
    #endif
}

// XXX those 3 below are probably implementation defined
inline int16_t byteswapI16(int16_t value) {
    return (value >> 8) | ((value & 0xFF) << 8);
}

inline int32_t byteswapI32(int32_t value) {
    return ((value >> 24) & 0xFF) | ((value >> 8) & 0xFF00) | ((value << 8) & 0xFF0000) | ((value << 24) & 0xFF000000);
}

inline int64_t byteswapI64(int64_t value) {
    return ((value >> 56) & 0xFF) | ((value >> 40) & 0xFF00) | ((value >> 24) & 0xFF0000) | ((value >> 8) & 0xFF000000) |
        ((value << 8) & 0xFF00000000LL) | ((value << 24) & 0xFF0000000000LL) | ((value << 40) & 0xFF000000000000LL) |
        ((value << 56) & 0xFF00000000000000LL);
}

// chatgpt mess might not work
inline float byteswapF32(float val) {
    float copy = val;
    unsigned char *bytes = reinterpret_cast<unsigned char*>(&copy);
    unsigned char temp;
    
    temp = bytes[0];
    bytes[0] = bytes[3];
    bytes[3] = temp;
    
    temp = bytes[1];
    bytes[1] = bytes[2];
    bytes[2] = temp;

    return copy;
}

inline double byteswapF64(double val) {
    double copy = val;

    unsigned char *bytes = reinterpret_cast<unsigned char*>(&copy);
    unsigned char temp;
    
    temp = bytes[0];
    bytes[0] = bytes[7];
    bytes[7] = temp;
    
    temp = bytes[1];
    bytes[1] = bytes[6];
    bytes[6] = temp;
    
    temp = bytes[2];
    bytes[2] = bytes[5];
    bytes[5] = temp;
    
    temp = bytes[3];
    bytes[3] = bytes[4];
    bytes[4] = temp;

    return copy;
}

class ByteBuffer {
public:
    ByteBuffer();
    ByteBuffer(const std::vector<uint8_t>& data);
    ByteBuffer(const char* data, size_t length);
    
    // Reading methods for various data types
    uint8_t readU8();
    int8_t readI8();
    uint16_t readU16();
    int16_t readI16();
    uint32_t readU32();
    int32_t readI32();
    uint64_t readU64();
    int64_t readI64();
    float readF32();
    double readF64();
    std::string readString();
    
    // Writing methods for various data types
    void writeU8(uint8_t value);
    void writeI8(int8_t value);
    void writeU16(uint16_t value);
    void writeI16(int16_t value);
    void writeU32(uint32_t value);
    void writeI32(int32_t value);
    void writeU64(uint64_t value);
    void writeI64(int64_t value);
    void writeF32(float value);
    void writeF64(double value);
    void writeString(const std::string& str);
    
    // Get the internal data as a vector
    std::vector<uint8_t> getData() const;
    
    size_t size() const;
    void clear();
    
    // Get the current position in the buffer
    size_t getPosition() const;
    void setPosition(size_t pos);

private:
    std::vector<uint8_t> data_;
    size_t position_;
    
    template <typename T>
    T read();
    
    template <typename T>
    void write(T value);

};