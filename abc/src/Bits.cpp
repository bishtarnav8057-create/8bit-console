#include<cstdio>
#include "Bits.h"

bool getBit(Byte value, int n){
	if(n < 0 || n > 7){
		printf("ERROR : bit %d does not exist (use 0-7)\n", n);
		return false;
	}
	return (value >> n) & 1;
}

Byte setBit(Byte value, int n){
	if(n < 0  || n >7){
		return value;
	}

		return (value >> n) & 1;
}
