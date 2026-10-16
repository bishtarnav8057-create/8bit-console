#include "IoPorts.h"
#include "MemoryMap.h"

namespace{
	const Word DEFAULT_SEED = 0xACE1;		//any non-zero start works; this is the classic one

	//port offsets inside the chip, from the addresses in MemoryMap.h
	const Word OFF_INPUT       = MemoryMap::PORT_INPUT       - MemoryMap::IO_START;
	const Word OFF_RNG         = MemoryMap::PORT_RNG         - MemoryMap::IO_START;
	const Word OFF_PPU_CTRL    = MemoryMap::PORT_PPU_CTRL    - MemoryMap::IO_START;
	const Word OFF_FRAME       = MemoryMap::PORT_FRAME       - MemoryMap::IO_START;
	const Word OFF_CONSOLE_OUT = MemoryMap::PORT_CONSOLE_OUT - MemoryMap::IO_START;
}

IoPorts::IoPorts(){
	reset();
}

void IoPorts::reset(){
	input = 0;
	frame = 0;
	rng = DEFAULT_SEED;
	ppuControl = 0;
	consoleText.clear();
}

Byte IoPorts::peek(Word offset){
	switch(offset){
		case OFF_INPUT:       return input;
		case OFF_RNG:         return lowByte(rng);
		case OFF_PPU_CTRL:    return ppuControl;
		case OFF_FRAME:       return frame;
		case OFF_CONSOLE_OUT: return 0;
		default:              return 0xFF;
	}
}

//only the RNG port changes when it is read. 8 steps = 8 fresh bits, so a number is not
//just the last one shifted by one. (8 and 65535 share no factor, so all states are still visited.)
Byte IoPorts::read(Word offset){
	if(offset == OFF_RNG){
		for(int i = 0; i < 8; ++i) stepRandom();
	}
	return peek(offset);
}

bool IoPorts::write(Word offset, Byte value){
	switch(offset){
		case OFF_RNG:
			seedRandom(makeWord(value, value));
			return true;
		case OFF_PPU_CTRL:
			ppuControl = value;
			if(ppuListener) ppuListener(value);
			return true;
		case OFF_CONSOLE_OUT:
			consoleText += (char)value;
			if(consoleListener) consoleListener((char)value);
			return true;
		default:
			return false;			//INPUT and FRAME are read-only, other offsets have no port
	}
}

unsigned int IoPorts::size() const{
	return (unsigned int)(MemoryMap::IO_END - MemoryMap::IO_START + 1);
}

const char* IoPorts::name() const{
	return "IO";
}

void IoPorts::setButton(Button button, bool held){
	input = held ? setBit(input, button) : clearBit(input, button);
}

void IoPorts::setInput(Byte buttons){
	input = buttons;
}

Byte IoPorts::getInput() const{
	return input;
}

void IoPorts::nextFrame(){
	frame = (Byte)(frame + 1);			//wraps 255 -> 0, programs only check "did it change?"
}

Byte IoPorts::getFrame() const{
	return frame;
}

//0 would get the generator stuck at 0 forever, so it is swapped for the default
void IoPorts::seedRandom(Word seed){
	rng = (seed == 0) ? DEFAULT_SEED : seed;
}

Word IoPorts::getRandomState() const{
	return rng;
}

//16-bit Galois LFSR: shift right, and if a 1 fell out, flip the tap bits.
//Goes through all 65535 non-zero values before repeating.
void IoPorts::stepRandom(){
	bool out = (rng & 1) != 0;
	rng = (Word)(rng >> 1);
	if(out) rng = (Word)(rng ^ 0xB400);
}

Byte IoPorts::getPpuControl() const{
	return ppuControl;
}

void IoPorts::setPpuListener(std::function<void(Byte)> listener){
	ppuListener = listener;
}

const std::string& IoPorts::getConsoleText() const{
	return consoleText;
}

void IoPorts::clearConsoleText(){
	consoleText.clear();
}

void IoPorts::setConsoleListener(std::function<void(char)> listener){
	consoleListener = listener;
}
