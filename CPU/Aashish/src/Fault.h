#ifndef FAULT_H
#define FAULT_H

#include <string>
#include "Bits.h"

enum class FaultType {
	None,
	InvalidOpcode,		//opcode byte has no entry in the table
	InvalidRegister,	//register number is bigger than 7
	IllegalWrite,		//tried to write into ROM
	UnmappedAccess,		//address belongs to no device
	StackOverflow,		//PUSH when the stac is full
	StackUnderflow,		//POP when the stack is empty
	InvalidJumpTarget	//JMP / CALL / RET going outside ROM
};

struct Fault{
	FaultType type;
	Word	pc;		//address of the instruction that failed
	Word	address;	//memory address involved (shows 0 if none)
	std::string message;

	Fault() : type(FaultType::None), pc(0), address(0) {}

};

const char * faultName(FaultType type);

#endif
