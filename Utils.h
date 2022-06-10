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
    byte size = 0;
    void clear();
    boolean append(byte b);
    boolean appendBuffer(const byte* buffer, size_t n);
    boolean append24(uint32_t value);
    boolean append32(uint32_t value);
    boolean pad(size_t n);
    boolean replace(size_t start, const byte* buffer, size_t n);
};
