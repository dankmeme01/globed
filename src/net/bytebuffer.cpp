#include "bytebuffer.hpp"

ByteBuffer::ByteBuffer() : position_(0) {}

ByteBuffer::ByteBuffer(const std::vector<uint8_t>& data) : data_(data), position_(0) {}
ByteBuffer::ByteBuffer(const char* data, size_t length) : data_(std::vector<uint8_t>(data, data + length)), position_(0) {}

template <typename T>
T ByteBuffer::read() {
    if (position_ + sizeof(T) > data_.size()) {
        throw std::runtime_error("ByteBuffer read beyond end of buffer");
    }
    T value;
    std::memcpy(&value, data_.data() + position_, sizeof(T));
    position_ += sizeof(T);
    return value;
}

template <typename T>
void ByteBuffer::write(T value) {
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
    data_.insert(data_.end(), bytes, bytes + sizeof(T));
    position_ += sizeof(T);
}

uint8_t ByteBuffer::readU8() {
    return read<uint8_t>();
}

int8_t ByteBuffer::readI8() {
    return read<int8_t>();
}

uint16_t ByteBuffer::readU16() {
    auto val = read<uint16_t>();
    if (GLOBED_LITTLE_ENDIAN) {
        val = byteswapU16(val);
    }
    return val;
}

int16_t ByteBuffer::readI16() {
    auto val = read<int16_t>();
    if (GLOBED_LITTLE_ENDIAN) {
        val = byteswapI16(val);
    }
    return val;
}

uint32_t ByteBuffer::readU32() {
    auto val = read<uint32_t>();
    if (GLOBED_LITTLE_ENDIAN) {
        val = byteswapU32(val);
    }
    return val;
}

int32_t ByteBuffer::readI32() {
    auto val = read<int32_t>();
    if (GLOBED_LITTLE_ENDIAN) {
        val = byteswapI32(val);
    }
    return val;
}

uint64_t ByteBuffer::readU64() {
    auto val = read<uint64_t>();
    if (GLOBED_LITTLE_ENDIAN) {
        val = byteswapU64(val);
    }
    return val;
}

int64_t ByteBuffer::readI64() {
    auto val = read<int64_t>();
    if (GLOBED_LITTLE_ENDIAN) {
        val = byteswapI64(val);
    }
    return val;
}

float ByteBuffer::readF32() {
    auto val = read<float>();
    if (GLOBED_LITTLE_ENDIAN) {
        val = byteswapF32(val);
    }
    return val;
}

double ByteBuffer::readF64() {
    auto val = read<double>();
    if (GLOBED_LITTLE_ENDIAN) {
        val = byteswapF64(val);
    }
    return val;
}

std::string ByteBuffer::readString() {
    auto length = readU32();
    std::string str(reinterpret_cast<const char*>(data_.data() + position_), length);
    position_ += length;
    return str;
}

void ByteBuffer::writeU8(uint8_t value) {
    write(value);
}

void ByteBuffer::writeI8(int8_t value) {
    write(value);
}

void ByteBuffer::writeU16(uint16_t value) {
    if (GLOBED_LITTLE_ENDIAN) {
        value = byteswapU16(value);
    }
    write(value);
}

void ByteBuffer::writeI16(int16_t value) {
    if (GLOBED_LITTLE_ENDIAN) {
        value = byteswapI16(value);
    }
    write(value);
}

void ByteBuffer::writeU32(uint32_t value) {
    if (GLOBED_LITTLE_ENDIAN) {
        value = byteswapU32(value);
    }
    write(value);
}

void ByteBuffer::writeI32(int32_t value) {
    if (GLOBED_LITTLE_ENDIAN) {
        value = byteswapI32(value);
    }
    write(value);
}

void ByteBuffer::writeU64(uint64_t value) {
    if (GLOBED_LITTLE_ENDIAN) {
        value = byteswapU64(value);
    }
    write(value);
}

void ByteBuffer::writeI64(int64_t value) {
    if (GLOBED_LITTLE_ENDIAN) {
        value = byteswapI64(value);
    }
    write(value);
}

void ByteBuffer::writeF32(float value) {
    if (GLOBED_LITTLE_ENDIAN) {
        value = byteswapF32(value);
    }
    write(value);
}

void ByteBuffer::writeF64(double value) {
    if (GLOBED_LITTLE_ENDIAN) {
        value = byteswapF64(value);
    }
    write(value);
}

void ByteBuffer::writeString(const std::string& str) {
    writeU32(str.size());
    data_.insert(data_.end(), str.begin(), str.end());
    position_ += str.size();
}

std::vector<uint8_t> ByteBuffer::getData() const {
    return data_;
}

void ByteBuffer::clear() {
    data_.clear();
    position_ = 0;
}

size_t ByteBuffer::size() const {
    return data_.size();
}

size_t ByteBuffer::getPosition() const {
    return position_;
}

void ByteBuffer::setPosition(size_t pos) {
    position_ = pos;
}