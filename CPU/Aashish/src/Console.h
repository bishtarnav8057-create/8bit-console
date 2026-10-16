//THE WHOLE MACHINE: EVERY CHIP WIRED TO THE BUS EXACTLY LIKE MemoryMap.h

#ifndef CONSOLE_H
#define CONSOLE_H

#include <vector>
#include "Bits.h"
#include "Rom.h"
#include "Ram.h"
#include "IoPorts.h"
#include "Bus.h"
#include "CPU.h"

//the one place the other units plug in:
//  Arnav (assembler / emulation controller): loadProgram(assembler bytes), step(), runFrame()
//  Ashmit (PPU / input): pixel(x, y) or vram(), io().setButton(...), io().setPpuListener(...)
//  Piyush (State Manager): bus().setWriteListener(...) to record, bus().poke(...) and cpu().set...() to undo
class Console{
	public:
		//how much the CPU may do between two frames. Programs wait for PORT_FRAME to change
		//to run once per frame, so this only has to be "enough" for one frame of game logic.
		static const unsigned long CYCLES_PER_FRAME = 20000;

		Console();

		bool loadProgram(const Byte* program, unsigned int length);	//false if bigger than ROM
		bool loadProgram(const std::vector<Byte>& program);
		void reset();							//clears RAM, VRAM, stack and ports, restarts the CPU; ROM is kept

		StepResult step();						//one instruction
		StepResult runFrame(unsigned long cycleBudget = CYCLES_PER_FRAME);	//one frame, then FRAME port +1

		Byte pixel(int x, int y);				//VRAM pixel, 0 outside the screen

		Rom&     rom();
		Ram&     ram();
		Ram&     vram();
		Ram&     stack();
		IoPorts& io();
		Bus&     bus();
		CPU&     cpu();

	private:
		//order matters: members are built top to bottom, and the CPU needs the Bus,
		//which needs the chips to exist first
		Rom     romChip;
		Ram     ramChip;
		Ram     vramChip;
		IoPorts ioChip;
		Ram     stackChip;
		Bus     theBus;
		CPU     theCpu;
};

#endif
