//THE INSTRUCTION DICTIONARY

#ifndef OPCODES_H
#define OPCODES_H

#include <string>
#include "Bits.h"

enum Opcode : Byte {
	//system
	OP_NOP  = 0x00, OP_HLT  = 0x01,
	//data movement
	OP_MOV  = 0x10, OP_LDI  = 0x11, OP_LD   = 0x12, OP_ST   = 0x13,
	OP_LDP  = 0x14, OP_STP  = 0x15, OP_INP  = 0x16, OP_DEP  = 0x17,
	//arithmetic and logic, register-register
	OP_ADD  = 0x30, OP_ADC  = 0x31, OP_SUB  = 0x32, OP_SBC  = 0x33,
	OP_AND  = 0x34, OP_OR   = 0x35, OP_XOR  = 0x36, OP_CMP  = 0x37,
	//arithmetic, register-immediate
	OP_ADDI = 0x38, OP_SUBI = 0x39, OP_CMPI = 0x3A,
	//single register
	OP_INC  = 0x40, OP_DEC  = 0x41, OP_NOT  = 0x42, OP_SHL  = 0x43, OP_SHR = 0x44,
	//control flow
	OP_JMP  = 0x60, OP_JZ   = 0x61, OP_JNZ  = 0x62, OP_JC   = 0x63,
	OP_JNC  = 0x64, OP_JN   = 0x65, OP_CALL = 0x66, OP_RET  = 0x67,
	//stack
	OP_PUSH = 0x80, OP_POP  = 0x81, OP_PUSHF = 0x82, OP_POPF = 0x83
};

enum class Format {
	Implied,	//opcode                      1 byte   NOP, HLT, RET
	Reg,		//opcode, reg                 2 bytes  INC R1, PUSH R3
	RegReg,		//opcode, (dst<<4 | src)      2 bytes  ADD R0, R1
	RegImm,		//opcode, reg, value          3 bytes  LDI R0, 42
	Abs,		//opcode, addrLow, addrHigh   3 bytes  JMP 0x0010
	RegAbs,		//opcode, reg, low, high      4 bytes  LD R0, [0x8000]
	RegInd		//opcode, reg (address in P)  2 bytes  LDP R0
};

struct OpcodeInfo {
	const char* mnemonic;
	Format      format;
	Byte        length;		//bytes, including the opcode
	Byte        cycles;
	bool        valid;
};

class OpcodeTable{
	public:
		static const OpcodeInfo& lookup(Byte opcode);
		static int  find(const std::string& mnemonic);		//opcode number, or -1
		static Byte lengthOf(Format format);
};

#endif
