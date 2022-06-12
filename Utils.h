#pragma once
#include <Arduino.h>

void dumpInfo(byte*, int);

uint32_t parseAppId(const byte* buffer);

#define CHECK_SIZE(s) \
    if (size + s - 1 >= N) return false;

template <size_t N>
class Buffer {
   public:
    byte buffer[N] = {0};
    size_t size = 0;
    void clear();
    boolean append(byte b);
    boolean appendBuffer(const byte* buffer, byte n);
    boolean append24(uint32_t value);
    boolean append32(uint32_t value);
    boolean pad(byte n);
    boolean replace(byte start, const byte* buffer, byte n);
};

template <size_t N>
void Buffer<N>::clear() {
    memset(buffer, 0, N);
    size = 0;
}
template <size_t N>
boolean Buffer<N>::append(byte b) {
    CHECK_SIZE(1)
    buffer[size] = b;
    size++;
    return true;
}
template <size_t N>
boolean Buffer<N>::appendBuffer(const byte* buffer, byte n) {
    CHECK_SIZE(n)
    memcpy(&this->buffer[size], buffer, n);
    size += n;
    return true;
}
template <size_t N>
boolean Buffer<N>::append24(uint32_t value) {
    CHECK_SIZE(3)
    buffer[size] = (value >> 0) & 0xff;
    buffer[size + 1] = (value >> 8) & 0xff;
    buffer[size + 2] = (value >> 16) & 0xff;
    size += 3;
    return true;
}
template <size_t N>
boolean Buffer<N>::append32(uint32_t value) {
    CHECK_SIZE(4)
    buffer[size] = (value >> 0) & 0xff;
    buffer[size + 1] = (value >> 8) & 0xff;
    buffer[size + 2] = (value >> 16) & 0xff;
    buffer[size + 3] = (value >> 24) & 0xff;
    size += 4;
    return true;
}
template <size_t N>
boolean Buffer<N>::pad(byte n) {
    CHECK_SIZE(n)
    size += n;
    return true;
}
template <size_t N>
boolean Buffer<N>::replace(byte start, const byte* buffer, byte n) {
    if (start + n >= N) {
        return false;
    }
    memcpy(&this->buffer[start], buffer, n);
    return false;
}
