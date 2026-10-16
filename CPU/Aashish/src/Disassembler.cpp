#include "Disassembler.h"
#include "Opcodes.h"
#include <cstdio>

Disassembler::Disassembler(Bus& theBus) : bus(theBus) {}

std::string Disassembler::line(Word address, Byte& length){
	//looking must not disturb the machine: a fault caused only by reading here is wiped afterwards
	bool hadFault = bus.hasFault();

	Byte opcode = bus.read(address);
	const OpcodeInfo& info = OpcodeTable::lookup(opcode);
	std::string text;

	if(!info.valid){
		length = 1;
		text = "DB " + hex8(opcode);		//"data byte": not an instruction
	}
	else{
		length = info.length;
		text = info.mnemonic;
		Byte b1 = (info.length > 1) ? bus.read((Word)(address + 1)) : 0;
		Byte b2 = (info.length > 2) ? bus.read((Word)(address + 2)) : 0;
		Byte b3 = (info.length > 3) ? bus.read((Word)(address + 3)) : 0;

		switch(info.format){
			case Format::Implied:
				break;
			case Format::Reg:
			case Format::RegInd:
				text += " " + regName(b1);
				break;
			case Format::RegReg:
				text += " " + regName(b1 >> 4) + ", " + regName(b1 & 0x0F);
				break;
			case Format::RegImm:
				text += " " + regName(b1) + ", " + hex8(b2);
				break;
			case Format::Abs:
				text += " " + hex16(makeWord(b1, b2));
				break;
			case Format::RegAbs:
				text += " " + regName(b1) + ", [" + hex16(makeWord(b2, b3)) + "]";
				break;
		}
	}

	if(!hadFault && bus.hasFault()) bus.clearFault();
	return hex16(address) + "  " + text;
}

std::string Disassembler::regName(int index){
	char text[8];
	snprintf(text, sizeof(text), "R%d", index);
	return text;
}

std::string Disassembler::hex8(Byte value){
	char text[8];
	snprintf(text, sizeof(text), "0x%02X", (unsigned int)value);
	return text;
}

std::string Disassembler::hex16(Word value){
	char text[8];
	snprintf(text, sizeof(text), "0x%04X", (unsigned int)value);
	return text;
}
