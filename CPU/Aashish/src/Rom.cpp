#include "Rom.h"
#include <algorithm>

Rom::Rom(unsigned int bytes) : data(bytes, 0) {}

Byte Rom::read(Word offset){
	if(offset >= data.size()) return 0xFF;
	return data[offset];
}

//the CPU can never change ROM; the Bus turns this "false" into an IllegalWrite fault
bool Rom::write(Word, Byte){
	return false;
}

unsigned int Rom::size() const{
	return (unsigned int)data.size();
}

const char* Rom::name() const{
	return "ROM";
}

//copies the program to offset 0 and zeroes the rest, so an older, longer program leaves nothing behind
bool Rom::load(const Byte* program, unsigned int length){
	if(length > data.size()) return false;
	if(program == nullptr && length > 0) return false;

	std::fill(data.begin(), data.end(), 0);
	std::copy(program, program + length, data.begin());
	return true;
}
