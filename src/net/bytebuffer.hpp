#pragma once

#include <vector>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>

class ByteBuffer {
public:
    ByteBuffer();

    void write_u8(uint8_t value);
    uint8_t read_u8();

    void write_i8(int8_t value);
    int8_t read_i8();

    void write_u16(uint16_t value);
    uint16_t read_u16();

    void write_i16(int16_t value);
    int16_t read_i16();

    void write_u32(uint32_t value);
    uint32_t read_u32();

    void write_i32(int32_t value);
    int32_t read_i32();

    void write_u64(uint64_t value);
    uint64_t read_u64();

    void write_i64(int64_t value);
    int64_t read_i64();

    void write_f32(float value);
    float read_f32();

    void write_f64(double value);
    double read_f64();

    void write_string(const std::string& str);
    std::string read_string();

    size_t size() const;
    std::vector<uint8_t> get_contents() const;
    void clear();

private:
    std::vector<uint8_t> buffer;
    size_t position; // Internal position for reading

    void write_bytes(const void* data, size_t size);

    template <typename T>
    T read_bytes();
};
