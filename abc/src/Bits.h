#ifndef BITS_H
#define BITS_H

#include <cstdint>

typedef uint8_t Byte;    //defining 8 bits 0 - 255
typedef uint16_t Word;   //defining 16 bits 0 - 65535 includes letters also

bool getBit(Byte value, int n);
Byte setBit(Byte value, int n);
#endif
