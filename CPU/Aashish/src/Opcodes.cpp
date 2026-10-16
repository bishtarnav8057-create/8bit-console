#include "Opcodes.h"
#include <cctype>

namespace{
	struct Entry{
		Byte        opcode;
		const char* mnemonic;
		Format      format;
		Byte        cycles;
	};

	//the one list of all 38 instructions. Length is not typed here: it comes from lengthOf(format),
	//so it can never disagree with the format.
	//cycles = bytes fetched + extra memory reads/writes the instruction makes
	const Entry ENTRIES[] = {
		{OP_NOP,   "NOP",   Format::Implied, 1},
		{OP_HLT,   "HLT",   Format::Implied, 1},

		{OP_MOV,   "MOV",   Format::RegReg,  2},
		{OP_LDI,   "LDI",   Format::RegImm,  3},
		{OP_LD,    "LD",    Format::RegAbs,  5},
		{OP_ST,    "ST",    Format::RegAbs,  5},
		{OP_LDP,   "LDP",   Format::RegInd,  3},
		{OP_STP,   "STP",   Format::RegInd,  3},
		{OP_INP,   "INP",   Format::Implied, 1},
		{OP_DEP,   "DEP",   Format::Implied, 1},

		{OP_ADD,   "ADD",   Format::RegReg,  2},
		{OP_ADC,   "ADC",   Format::RegReg,  2},
		{OP_SUB,   "SUB",   Format::RegReg,  2},
		{OP_SBC,   "SBC",   Format::RegReg,  2},
		{OP_AND,   "AND",   Format::RegReg,  2},
		{OP_OR,    "OR",    Format::RegReg,  2},
		{OP_XOR,   "XOR",   Format::RegReg,  2},
		{OP_CMP,   "CMP",   Format::RegReg,  2},

		{OP_ADDI,  "ADDI",  Format::RegImm,  3},
		{OP_SUBI,  "SUBI",  Format::RegImm,  3},
		{OP_CMPI,  "CMPI",  Format::RegImm,  3},

		{OP_INC,   "INC",   Format::Reg,     2},
		{OP_DEC,   "DEC",   Format::Reg,     2},
		{OP_NOT,   "NOT",   Format::Reg,     2},
		{OP_SHL,   "SHL",   Format::Reg,     2},
		{OP_SHR,   "SHR",   Format::Reg,     2},

		{OP_JMP,   "JMP",   Format::Abs,     3},
		{OP_JZ,    "JZ",    Format::Abs,     3},
		{OP_JNZ,   "JNZ",   Format::Abs,     3},
		{OP_JC,    "JC",    Format::Abs,     3},
		{OP_JNC,   "JNC",   Format::Abs,     3},
		{OP_JN,    "JN",    Format::Abs,     3},
		{OP_CALL,  "CALL",  Format::Abs,     5},
		{OP_RET,   "RET",   Format::Implied, 3},

		{OP_PUSH,  "PUSH",  Format::Reg,     3},
		{OP_POP,   "POP",   Format::Reg,     3},
		{OP_PUSHF, "PUSHF", Format::Implied, 2},
		{OP_POPF,  "POPF",  Format::Implied, 2}
	};
	const int ENTRY_COUNT = sizeof(ENTRIES) / sizeof(ENTRIES[0]);

	//256 slots, one per possible byte, so lookup is a plain array index: O(1)
	struct Table{
		OpcodeInfo slot[256];

		Table(){
			OpcodeInfo invalid = {"???", Format::Implied, 1, 0, false};
			for(int i = 0; i < 256; ++i) slot[i] = invalid;

			for(int i = 0; i < ENTRY_COUNT; ++i){
				const Entry& e = ENTRIES[i];
				OpcodeInfo info = {e.mnemonic, e.format, OpcodeTable::lengthOf(e.format), e.cycles, true};
				slot[e.opcode] = info;
			}
		}
	};

	//built the first time it is needed (safe even if another file's globals call lookup early)
	const Table& table(){
		static const Table t;
		return t;
	}

	bool sameIgnoringCase(const std::string& a, const char* b){
		size_t i = 0;
		for(; i < a.size() && b[i] != '\0'; ++i){
			if(std::toupper((unsigned char)a[i]) != std::toupper((unsigned char)b[i])) return false;
		}
		return i == a.size() && b[i] == '\0';
	}
}

const OpcodeInfo& OpcodeTable::lookup(Byte opcode){
	return table().slot[opcode];
}

//case-insensitive, so "add", "Add" and "ADD" from the parser all work
int OpcodeTable::find(const std::string& mnemonic){
	for(int i = 0; i < ENTRY_COUNT; ++i){
		if(sameIgnoringCase(mnemonic, ENTRIES[i].mnemonic)) return ENTRIES[i].opcode;
	}
	return -1;
}

Byte OpcodeTable::lengthOf(Format format){
	switch(format){
		case Format::Implied:	return 1;
		case Format::Reg:		return 2;
		case Format::RegReg:	return 2;
		case Format::RegImm:	return 3;
		case Format::Abs:		return 3;
		case Format::RegAbs:	return 4;
		case Format::RegInd:	return 2;
	}
	return 1;
}
