#include "Bits.h"

//bits outside 0-7 do not exist in a Byte: getBit says false, set/clear leave the value alone

bool getBit(Byte value, int n){
	if(n < 0 || n > 7) return false;
	return (value >> n) & 1;
}

Byte setBit(Byte value, int n){
	if(n < 0 || n > 7) return value;
	return (Byte)(value | (1 << n));
}

Byte clearBit(Byte value, int n){
	if(n < 0 || n > 7) return value;
	return (Byte)(value & ~(1 << n));
}

Byte lowByte(Word w){
	return (Byte)(w & 0xFF);
}

Byte highByte(Word w){
	return (Byte)((w >> 8) & 0xFF);
}

Word makeWord(Byte low, Byte high){
	return (Word)(low | (high << 8));
}
