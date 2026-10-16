#include "Ram.h"
#include <algorithm>

Ram::Ram(unsigned int bytes, const char* chipName) : data(bytes, 0), label(chipName) {}

//the Bus never passes an offset past the end, but a stray one is refused instead of crashing
Byte Ram::read(Word offset){
	if(offset >= data.size()) return 0xFF;
	return data[offset];
}

bool Ram::write(Word offset, Byte value){
	if(offset >= data.size()) return false;
	data[offset] = value;
	return true;
}

unsigned int Ram::size() const{
	return (unsigned int)data.size();
}

const char* Ram::name() const{
	return label;
}

void Ram::clear(){
	std::fill(data.begin(), data.end(), 0);
}
