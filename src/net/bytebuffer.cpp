#include "bytebuffer.hpp"

ByteBuffer::ByteBuffer() : position(0) {}

void ByteBuffer::write_bytes(const void* data, size_t size) {
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    buffer.insert(buffer.end(), bytes, bytes + size);
}

template <typename T>
T ByteBuffer::read_bytes() {
    if (position + sizeof(T) <= buffer.size()) {
        const T* value_ptr = reinterpret_cast<const T*>(&buffer[position]);
        position += sizeof(T);
        return *value_ptr;
    } else {
        throw std::out_of_range("Index out of bounds");
    }
}

void ByteBuffer::write_u8(uint8_t value) {
    write_bytes(&value, sizeof(value));
}

uint8_t ByteBuffer::read_u8() {
    return read_bytes<uint8_t>();
}

void ByteBuffer::write_i8(int8_t value) {
    write_bytes(&value, sizeof(value));
}

int8_t ByteBuffer::read_i8() {
    return read_bytes<int8_t>();
}

void ByteBuffer::write_u16(uint16_t value) {
    write_bytes(&value, sizeof(value));
}

uint16_t ByteBuffer::read_u16() {
    return read_bytes<uint16_t>();
}

void ByteBuffer::write_i16(int16_t value) {
    write_bytes(&value, sizeof(value));
}

int16_t ByteBuffer::read_i16() {
    return read_bytes<int16_t>();
}

void ByteBuffer::write_u32(uint32_t value) {
    write_bytes(&value, sizeof(value));
}

uint32_t ByteBuffer::read_u32() {
    return read_bytes<uint32_t>();
}

void ByteBuffer::write_i32(int32_t value) {
    write_bytes(&value, sizeof(value));
}

int32_t ByteBuffer::read_i32() {
    return read_bytes<int32_t>();
}

void ByteBuffer::write_u64(uint64_t value) {
    write_bytes(&value, sizeof(value));
}

uint64_t ByteBuffer::read_u64() {
    return read_bytes<uint64_t>();
}

void ByteBuffer::write_i64(int64_t value) {
    write_bytes(&value, sizeof(value));
}

int64_t ByteBuffer::read_i64() {
    return read_bytes<int64_t>();
}

void ByteBuffer::write_f32(float value) {
    write_bytes(&value, sizeof(value));
}

float ByteBuffer::read_f32() {
    return read_bytes<float>();
}

void ByteBuffer::write_f64(double value) {
    write_bytes(&value, sizeof(value));
}

double ByteBuffer::read_f64() {
    return read_bytes<double>();
}

void ByteBuffer::write_string(const std::string& str) {
    write_u32(static_cast<uint32_t>(str.size()));
    write_bytes(str.c_str(), str.size());
}

std::string ByteBuffer::read_string() {
    uint32_t length = read_u32();

    if (position + length <= buffer.size()) {
        std::string result(reinterpret_cast<const char*>(&buffer[position]), length);
        position += length;
        return result;
    } else {
        throw std::out_of_range("Index out of bounds");
    }
}

size_t ByteBuffer::size() const {
    return buffer.size();
}

std::vector<uint8_t> ByteBuffer::get_contents() const {
    return buffer;
}

void ByteBuffer::clear() {
    buffer.clear();
    position = 0; // Reset position when clearing
}
