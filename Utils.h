#pragma once
#include <Arduino.h>

void dumpInfo(byte*, int);

uint32_t parseAppId(byte* buffer);
