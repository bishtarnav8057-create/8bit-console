#include "Console.h"
#include "MemoryMap.h"

namespace{
	unsigned int sizeOf(Word start, Word end){
		return (unsigned int)end - start + 1;
	}
}

const unsigned long Console::CYCLES_PER_FRAME;		//the value is in Console.h; this gives it an address

Console::Console()
	: romChip(sizeOf(MemoryMap::ROM_START, MemoryMap::ROM_END)),
	  ramChip(sizeOf(MemoryMap::RAM_START, MemoryMap::RAM_END), "RAM"),
	  vramChip(sizeOf(MemoryMap::VRAM_START, MemoryMap::VRAM_END), "VRAM"),
	  ioChip(),
	  stackChip(sizeOf(MemoryMap::STACK_START, MemoryMap::STACK_END), "STACK"),
	  theBus(),
	  theCpu(theBus){
	theBus.attach(MemoryMap::ROM_START,   MemoryMap::ROM_END,   &romChip);
	theBus.attach(MemoryMap::RAM_START,   MemoryMap::RAM_END,   &ramChip);
	theBus.attach(MemoryMap::VRAM_START,  MemoryMap::VRAM_END,  &vramChip);
	theBus.attach(MemoryMap::IO_START,    MemoryMap::IO_END,    &ioChip);
	theBus.attach(MemoryMap::STACK_START, MemoryMap::STACK_END, &stackChip);
}

//a new program starts on a clean machine, so nothing from the last one leaks in
bool Console::loadProgram(const Byte* program, unsigned int length){
	if(!romChip.load(program, length)) return false;
	reset();
	return true;
}

bool Console::loadProgram(const std::vector<Byte>& program){
	return loadProgram(program.empty() ? nullptr : program.data(), (unsigned int)program.size());
}

void Console::reset(){
	ramChip.clear();
	vramChip.clear();
	stackChip.clear();
	ioChip.reset();
	theCpu.reset(MemoryMap::ROM_START);
}

StepResult Console::step(){
	return theCpu.step();
}

//runs until the frame's cycles are used up, or the CPU halts or faults. The frame always ends,
//so the screen keeps refreshing even after a HLT.
StepResult Console::runFrame(unsigned long cycleBudget){
	unsigned long stopAt = theCpu.getCycles() + cycleBudget;
	StepResult result = StepResult::Ok;

	while(theCpu.getCycles() < stopAt){
		result = theCpu.step();
		if(result != StepResult::Ok) break;
	}

	ioChip.nextFrame();
	return result;
}

Byte Console::pixel(int x, int y){
	if(x < 0 || x >= MemoryMap::SCREEN_WIDTH || y < 0 || y >= MemoryMap::SCREEN_HEIGHT) return 0;
	return vramChip.peek((Word)(y * MemoryMap::SCREEN_WIDTH + x));
}

Rom&     Console::rom()   { return romChip; }
Ram&     Console::ram()   { return ramChip; }
Ram&     Console::vram()  { return vramChip; }
Ram&     Console::stack() { return stackChip; }
IoPorts& Console::io()    { return ioChip; }
Bus&     Console::bus()   { return theBus; }
CPU&     Console::cpu()   { return theCpu; }
