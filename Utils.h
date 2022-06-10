#pragma once
#include <Arduino.h>

void dumpInfo(byte*, int);

uint32_t parseAppId(byte* buffer);

#define CHECK_SIZE(s) \
    if (size + s - 1 >= N) return false;

template <size_t N>
class Buffer {
   public:
    byte buffer[N];
    size_t size = 0;
    boolean append(byte b);
    boolean append24(uint32_t value);
    boolean append32(uint32_t value);
};
