//test framework that checks answers automatically and says PASS OR FAIL

#include<cstdio>
#include "../src/Bits.h"  //setting up path for Bits header file

int passed = 0;
int failed = 0;

void check(bool condition, const char* name) {
	if(condition){
		printf("[PASS] %s\n" , name);
		passed++;
	}
	else{
		printf("[FAIL] %s\n", name);
		failed++;
	}
}

int main(){
	printf("--Bits Test--\n");
	check(getBit(0x04,2) == 1, "getBit: bit 2 of 0x04 is 1");
	check(getBit(0x04,0) == 0, "getBit: bit 0 of 0x04 is 0");
	check(getBit(0x80,7) == 1, "getBit: bit 7 of 0x80 is 1");
	check(setBit(0x00, 0) == 0x01, "setBit: set bit 0 of 0x00 -> 0x01");
	check(setBit(0x01, 3) == 0x09, "setBit: set bit 3 of 0x01 -> 0x09");
	check(setBit(0x01, 0) == 0x01, "setBit: setting a bit that is already 1");

	//wraparound
	Byte b = 255;
	b = b+1;
	check(b==0,"Byte 255 + 1 Wraps  to 0");

	printf("\n%d passed, %d failed\n",passed, failed);
	return failed ==  0 ? 0 : 1;

}
