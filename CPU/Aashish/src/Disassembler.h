//BYTES BACK INTO READABLE ASSEMBLY

#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include <string>
#include "Bus.h"

class Disassembler{
	public:
		explicit Disassembler(Bus& bus);

		std::string line(Word address, Byte& length);	//e.g. "0x0003  LDI R0, 0x2A"

	private:
		Bus& bus;

		static std::string regName(int index);
		static std::string hex8(Byte value);
		static std::string hex16(Word value);
};

#endif
