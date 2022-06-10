#include "Utils.h"

void dumpInfo(byte* ar, int len) {
    for (int i = 0; i < len; i++) {
        if (ar[i] < 0x10)
            Serial.print(F("0"));
        Serial.print(ar[i], HEX);
        Serial.print(F(" "));
    }
    Serial.println("");
}

uint32_t parseAppId(const byte* buffer) {
    return buffer[0] | buffer[1] << 8 | buffer[2] << 6;
}

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
boolean Buffer<N>::appendBuffer(const byte* buffer, size_t n) {
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
boolean Buffer<N>::pad(size_t n) {
    CHECK_SIZE(n)
    size += n;
    return true;
}
template <size_t N>
boolean Buffer<N>::replace(size_t start, const byte* buffer, size_t n) {
    if (start + n >= N) {
        return false;
    }
    memcpy(&this->buffer[start], buffer, n);
    return false;
}
