#ifndef BITS_H
#define BITS_H

#include <cstdint>

typedef uint8_t Byte;  //8 bits,  0 to 255
typedef uint16_t Word;  //16 bits, 0 to 65535 which is used for addreses

bool getBit(Byte value, int n);    //is bit n on?
Byte setBit(Byte value, int n);   //turn bit n on.
Byte clearBit(Byte value, int n);   //turn bit n off

Byte lowByte(Word w);    //bits 0-7 of a word
Byte highByte(Word w);   //bits 8-15 of a word
Word makeWord(Byte low, Byte high);   //join two bytes into on word

#endif
