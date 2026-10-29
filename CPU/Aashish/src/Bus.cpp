#include "Bus.h"
#include <cstdio>

namespace{
	std::string hex16(Word value){
		char text[8];
		snprintf(text, sizeof(text), "0x%04X", (unsigned int)value);
		return text;
	}
}

Bus::Bus() {}

//refuses a missing chip, a backwards range, a range bigger than the chip, or one that overlaps another chip
bool Bus::attach(Word start, Word end, Device* device){
	if(device == nullptr || start > end) return false;

	unsigned int rangeSize = (unsigned int)end - start + 1;		//unsigned int, so 0x0000-0xFFFF (65536) fits
	if(rangeSize > device->size()) return false;

	for(size_t i = 0; i < mappings.size(); ++i){
		if(start <= mappings[i].end && end >= mappings[i].start) return false;
	}

	Mapping m;
	m.start = start;
	m.end = end;
	m.device = device;

	//insert it in order of start address, so the list stays sorted for find()
	size_t pos = 0;
	while(pos < mappings.size() && mappings[pos].start < start) ++pos;
	mappings.insert(mappings.begin() + pos, m);
	return true;
}

//binary search: the ranges are sorted and never overlap, so look at the middle one and throw
//away the half the address cannot be in. O(log n) checks instead of trying every chip.
//This runs on every memory access the CPU makes, and attach() only runs at start-up.
Bus::Mapping* Bus::find(Word address){
	int low = 0;
	int high = (int)mappings.size() - 1;

	while(low <= high){
		int mid = low + (high - low) / 2;
		if(address < mappings[mid].start)    high = mid - 1;		//it is in the left half
		else if(address > mappings[mid].end) low = mid + 1;			//it is in the right half
		else return &mappings[mid];
	}
	return nullptr;			//it fell in a gap between chips
}

Byte Bus::read(Word address){
	Mapping* m = find(address);
	if(m == nullptr){
		raise(FaultType::UnmappedAccess, address, "No device at " + hex16(address));
		return 0xFF;
	}
	return m->device->read((Word)(address - m->start));
}

bool Bus::write(Word address, Byte value){
	Mapping* m = find(address);
	if(m == nullptr){
		raise(FaultType::UnmappedAccess, address, "No device at " + hex16(address));
		return false;
	}

	Word offset = (Word)(address - m->start);
	Byte oldValue = listener ? m->device->peek(offset) : 0;		//only needed when someone is listening

	if(!m->device->write(offset, value)){
		raise(FaultType::IllegalWrite, address,
			std::string("Write refused by ") + m->device->name() + " at " + hex16(address));
		return false;
	}

	if(listener) listener(address, oldValue, value);
	return true;
}

Byte Bus::peek(Word address){
	Mapping* m = find(address);
	if(m == nullptr) return 0xFF;
	return m->device->peek((Word)(address - m->start));
}

bool Bus::poke(Word address, Byte value){
	Mapping* m = find(address);
	if(m == nullptr) return false;
	return m->device->write((Word)(address - m->start), value);
}

Word Bus::readWord(Word address){
	Byte low = read(address);
	Byte high = read((Word)(address + 1));
	return makeWord(low, high);
}

bool Bus::writeWord(Word address, Word value){
	if(!write(address, lowByte(value))) return false;
	return write((Word)(address + 1), highByte(value));
}

bool Bus::hasFault() const{
	return fault.type != FaultType::None;
}

const Fault& Bus::getFault() const{
	return fault;
}

void Bus::clearFault(){
	fault = Fault();
}

void Bus::setWriteListener(WriteListener newListener){
	listener = newListener;
}

//keeps the FIRST fault until clearFault(), because the first error is the real cause.
//The Bus does not know the PC, so pc stays 0; the CPU fills it in when it copies the fault.
void Bus::raise(FaultType type, Word address, const std::string& message){
	if(hasFault()) return;
	fault.type = type;
	fault.pc = 0;
	fault.address = address;
	fault.message = message;
}
