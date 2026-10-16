#ifndef MEMORYMAP_H
#define MEMORYMAP_H

#include "Bits.h"

namespace MemoryMap{
	const Word ROM_START	= 0x0000;
	const Word ROM_END    	= 0x7fff;		//32 KB, ,program
	const Word RAM_START  	= 0x8000;
	const Word RAM_END    	= 0xDfff;		//24 KB, variables
	const Word VRAM_START	= 0xE000;
	const Word VRAM_END		= 0xEFFF;		//4 KB, screen pixels
	const Word IO_START 	= 0xF000;
	const Word IO_END 		= 0xF0FF;		//256 bytes, ports
	const Word STACK_START 	= 0xF100;		//lowest address the stack may use
	const Word STACK_END 	= 0xFFFF;		// SP starts here

	const Word PORT_INPUT 		= 0xF000;
	const Word PORT_RNG 		= 0xF001;
	const Word PORT_PPU_CTRL 	= 0xF002;
	const Word PORT_FRAME 		= 0xF003;
	const Word PORT_CONSOLE_OUT = 0xF004;

}

#endif
