//test framework that checks answers automatically and says PASS OR FAIL
//covers every class in CPU/Aashish/src: Bits, Fault, Ram, Rom, Bus, OpcodeTable, CPU, Disassembler

#include <cstdio>
#include <cstring>
#include <string>

#include "../src/Bits.h"
#include "../src/MemoryMap.h"
#include "../src/Fault.h"
#include "../src/Ram.h"
#include "../src/Rom.h"
#include "../src/Bus.h"
#include "../src/Opcodes.h"
#include "../src/CPU.h"
#include "../src/Disassembler.h"

int passed = 0;
int failed = 0;

void check(bool condition, const char* name){
	if(condition){
		printf("[PASS] %s\n", name);
		passed++;
	}
	else{
		printf("[FAIL] %s\n", name);
		failed++;
	}
}

//a full console wired exactly like the memory map (I/O ports 0xF000-0xF0FF are left for the PPU side)
struct Machine{
	Rom rom;
	Ram ram;
	Ram vram;
	Ram stack;
	Bus bus;
	CPU cpu;

	Machine() : rom(0x8000), ram(0x6000, "RAM"), vram(0x1000, "VRAM"), stack(0x0F00, "STACK"), bus(), cpu(bus){
		bus.attach(MemoryMap::ROM_START,   MemoryMap::ROM_END,   &rom);
		bus.attach(MemoryMap::RAM_START,   MemoryMap::RAM_END,   &ram);
		bus.attach(MemoryMap::VRAM_START,  MemoryMap::VRAM_END,  &vram);
		bus.attach(MemoryMap::STACK_START, MemoryMap::STACK_END, &stack);
	}

	StepResult runProgram(const Byte* program, unsigned int length){
		rom.load(program, length);
		cpu.reset();
		return cpu.run(100000);
	}
};

#define RUN(m, program) (m).runProgram(program, sizeof(program))

bool flagsAre(const CPU& cpu, int z, int c, int n, int v){
	return cpu.getFlag(CPU::FLAG_Z) == (z == 1) && cpu.getFlag(CPU::FLAG_C) == (c == 1)
		&& cpu.getFlag(CPU::FLAG_N) == (n == 1) && cpu.getFlag(CPU::FLAG_V) == (v == 1);
}

void testBits(){
	printf("--Bits--\n");
	check(getBit(0x04, 2) == 1, "getBit: bit 2 of 0x04 is 1");
	check(getBit(0x04, 0) == 0, "getBit: bit 0 of 0x04 is 0");
	check(getBit(0x80, 7) == 1, "getBit: bit 7 of 0x80 is 1");
	check(setBit(0x00, 0) == 0x01, "setBit: set bit 0 of 0x00 -> 0x01");
	check(setBit(0x01, 3) == 0x09, "setBit: set bit 3 of 0x01 -> 0x09");
	check(setBit(0x01, 0) == 0x01, "setBit: setting a bit that is already 1");
	check(clearBit(0x0F, 0) == 0x0E, "clearBit: clear bit 0 of 0x0F -> 0x0E");
	check(clearBit(0x80, 7) == 0x00, "clearBit: clear bit 7 of 0x80 -> 0x00");
	check(getBit(0xFF, 8) == false && setBit(0x00, 9) == 0x00 && clearBit(0xFF, -1) == 0xFF,
		"bits outside 0-7 are ignored");
	check(lowByte(0x1234) == 0x34 && highByte(0x1234) == 0x12, "lowByte / highByte of 0x1234");
	check(makeWord(0x34, 0x12) == 0x1234, "makeWord(0x34, 0x12) = 0x1234");
	Byte b = 255;
	b = b + 1;
	check(b == 0, "Byte 255 + 1 wraps to 0");
}

void testMemoryMap(){
	printf("--MemoryMap--\n");
	check(MemoryMap::VRAM_END - MemoryMap::VRAM_START + 1 == 0x1000, "VRAM is 4 KB");
	check(MemoryMap::RAM_END + 1 == MemoryMap::VRAM_START && MemoryMap::VRAM_END + 1 == MemoryMap::IO_START
		&& MemoryMap::IO_END + 1 == MemoryMap::STACK_START, "regions touch with no gaps");
}

void testFault(){
	printf("--Fault--\n");
	Fault f;
	check(f.type == FaultType::None && f.pc == 0 && f.address == 0, "new Fault means no error");
	check(std::strcmp(faultName(FaultType::StackOverflow), "StackOverflow") == 0, "faultName prints the type");
	check(std::strcmp(faultName(FaultType::InvalidJumpTarget), "InvalidJumpTarget") == 0, "faultName last type");
}

void testRamRom(){
	printf("--Ram / Rom--\n");
	Ram ram(16, "RAM");
	check(ram.size() == 16 && std::strcmp(ram.name(), "RAM") == 0, "Ram size and name");
	check(ram.read(5) == 0, "Ram starts zeroed");
	check(ram.write(5, 0xAB) && ram.read(5) == 0xAB, "Ram write then read");
	check(!ram.write(16, 1) && ram.read(16) == 0xFF, "Ram refuses offsets past its end");
	ram.clear();
	check(ram.read(5) == 0, "Ram clear zeroes everything");

	Rom rom(8);
	Byte program[] = {1, 2, 3};
	check(!rom.write(0, 7) && rom.read(0) == 0, "Rom refuses writes");
	check(rom.load(program, 3) && rom.read(0) == 1 && rom.read(2) == 3, "Rom load copies the program");
	Byte tooBig[9] = {0};
	check(!rom.load(tooBig, 9) && rom.read(0) == 1, "Rom refuses a program bigger than itself");
	Byte shorter[] = {9};
	rom.load(shorter, 1);
	check(rom.read(0) == 9 && rom.read(1) == 0, "loading a shorter program clears the old bytes");
	check(std::strcmp(rom.name(), "ROM") == 0 && rom.size() == 8, "Rom size and name");
}

Word  heardAddress = 0;
Byte  heardOld = 0, heardNew = 0;
int   heardCount = 0;

void testBus(){
	printf("--Bus--\n");
	Ram a(0x100, "A");
	Ram b(0x100, "B");
	Rom r(0x100);
	Bus bus;
	check(bus.attach(0x1000, 0x10FF, &a), "attach a chip");
	check(!bus.attach(0x10F0, 0x11EF, &b), "overlapping range is refused");
	check(!bus.attach(0x2000, 0x2200, &b), "range bigger than the chip is refused");
	check(!bus.attach(0x3000, 0x2000, &b), "backwards range is refused");
	check(!bus.attach(0x3000, 0x3001, nullptr), "missing chip is refused");
	check(bus.attach(0x2000, 0x20FF, &b) && bus.attach(0x0000, 0x00FF, &r), "attach two more chips");

	check(bus.write(0x2005, 0x77) && b.read(5) == 0x77 && a.read(5) == 0, "address becomes offset in the right chip");
	check(bus.read(0x2005) == 0x77 && !bus.hasFault(), "read goes to the right chip");

	check(bus.read(0x5000) == 0xFF && bus.hasFault(), "unmapped read gives 0xFF and a fault");
	check(bus.getFault().type == FaultType::UnmappedAccess && bus.getFault().address == 0x5000, "fault says UnmappedAccess and where");
	bus.write(0x0001, 1);
	check(bus.getFault().type == FaultType::UnmappedAccess, "the first fault is kept until cleared");
	bus.clearFault();
	check(!bus.hasFault(), "clearFault removes it");

	check(!bus.write(0x0001, 1) && bus.getFault().type == FaultType::IllegalWrite, "write to ROM gives IllegalWrite");
	bus.clearFault();

	check(bus.writeWord(0x1000, 0x1234) && a.read(0) == 0x34 && a.read(1) == 0x12, "writeWord is little-endian");
	check(bus.readWord(0x1000) == 0x1234, "readWord joins the bytes back");

	bus.setWriteListener([](Word address, Byte oldValue, Byte newValue){
		heardAddress = address; heardOld = oldValue; heardNew = newValue; heardCount++;
	});
	bus.write(0x1010, 0x05);
	bus.write(0x1010, 0x09);
	check(heardCount == 2 && heardAddress == 0x1010 && heardOld == 0x05 && heardNew == 0x09, "listener hears old and new value");
	bus.write(0x0001, 1);
	check(heardCount == 2, "listener is not called for a refused write");
}

void testOpcodes(){
	printf("--OpcodeTable--\n");
	int validCount = 0;
	bool lengthsMatch = true, findMatches = true;
	for(int i = 0; i < 256; ++i){
		const OpcodeInfo& info = OpcodeTable::lookup((Byte)i);
		if(!info.valid) continue;
		validCount++;
		if(info.length != OpcodeTable::lengthOf(info.format)) lengthsMatch = false;
		if(OpcodeTable::find(info.mnemonic) != i) findMatches = false;
	}
	check(validCount == 38, "exactly 38 valid opcodes");
	check(lengthsMatch, "every length matches its format");
	check(findMatches, "find(mnemonic) gives back the same opcode for all 38");

	const OpcodeInfo& add = OpcodeTable::lookup(0x30);
	check(add.valid && std::strcmp(add.mnemonic, "ADD") == 0 && add.format == Format::RegReg && add.length == 2, "0x30 is ADD, RegReg, 2 bytes");
	check(OpcodeTable::lookup(0x12).length == 4, "LD is 4 bytes");
	check(!OpcodeTable::lookup(0xFF).valid && !OpcodeTable::lookup(0x02).valid, "unused bytes are invalid");
	check(OpcodeTable::find("add") == 0x30 && OpcodeTable::find("Pushf") == 0x82, "find ignores upper/lower case");
	check(OpcodeTable::find("LOAD") == -1 && OpcodeTable::find("") == -1 && OpcodeTable::find("ADDX") == -1, "unknown mnemonic gives -1");
}

void testFlagTable(){
	printf("--CPU flags (booklet table)--\n");
	Machine m;

	Byte p1[] = {0x11, 0x00, 0xFF,  0x11, 0x01, 0x01,  0x30, 0x01,  0x01};		//0xFF + 0x01
	RUN(m, p1);
	check(m.cpu.getReg(0) == 0x00 && flagsAre(m.cpu, 1, 1, 0, 0), "0xFF + 0x01 = 0x00  Z C");

	Byte p2[] = {0x11, 0x00, 0x7F,  0x11, 0x01, 0x01,  0x30, 0x01,  0x01};		//0x7F + 0x01
	RUN(m, p2);
	check(m.cpu.getReg(0) == 0x80 && flagsAre(m.cpu, 0, 0, 1, 1), "0x7F + 0x01 = 0x80  N V");

	Byte p3[] = {0x11, 0x00, 0x05,  0x11, 0x01, 0x05,  0x32, 0x01,  0x01};		//0x05 - 0x05
	RUN(m, p3);
	check(m.cpu.getReg(0) == 0x00 && flagsAre(m.cpu, 1, 0, 0, 0), "0x05 - 0x05 = 0x00  Z");

	Byte p4[] = {0x11, 0x00, 0x00,  0x11, 0x01, 0x01,  0x32, 0x01,  0x01};		//0x00 - 0x01
	RUN(m, p4);
	check(m.cpu.getReg(0) == 0xFF && flagsAre(m.cpu, 0, 1, 1, 0), "0x00 - 0x01 = 0xFF  C N (borrow)");

	Byte p5[] = {0x11, 0x00, 0x80,  0x11, 0x01, 0x01,  0x32, 0x01,  0x01};		//0x80 - 0x01
	RUN(m, p5);
	check(m.cpu.getReg(0) == 0x7F && flagsAre(m.cpu, 0, 0, 0, 1), "0x80 - 0x01 = 0x7F  V");

	//edge cases: the exact numbers where carry/overflow just do or just do not happen
	Byte p6[] = {0x11, 0x00, 0xF0,  0x11, 0x01, 0x0F,  0x30, 0x01,  0x01};		//0xF0 + 0x0F
	RUN(m, p6);
	check(m.cpu.getReg(0) == 0xFF && flagsAre(m.cpu, 0, 0, 1, 0), "0xF0 + 0x0F = 0xFF  N, no carry (largest sum that fits)");

	Byte p7[] = {0x11, 0x00, 0x80,  0x11, 0x01, 0x80,  0x30, 0x01,  0x01};		//0x80 + 0x80
	RUN(m, p7);
	check(m.cpu.getReg(0) == 0x00 && flagsAre(m.cpu, 1, 1, 0, 1), "0x80 + 0x80 = 0x00  Z C V (-128 + -128)");

	Byte p8[] = {0x11, 0x00, 0x7F,  0x11, 0x01, 0xFF,  0x32, 0x01,  0x01};		//0x7F - 0xFF
	RUN(m, p8);
	check(m.cpu.getReg(0) == 0x80 && flagsAre(m.cpu, 0, 1, 1, 1), "0x7F - 0xFF = 0x80  C N V (127 - -1)");
}

void testInstructions(){
	printf("--CPU instructions--\n");
	Machine m;

	Byte adc[] = {0x11, 0x00, 0xFF, 0x11, 0x01, 0x01, 0x30, 0x01,		//C = 1
				  0x11, 0x02, 0x10, 0x11, 0x03, 0x20, 0x31, 0x23, 0x01};	//R2 = 0x10 + 0x20 + 1
	RUN(m, adc);
	check(m.cpu.getReg(2) == 0x31 && !m.cpu.getFlag(CPU::FLAG_C), "ADC adds the carry");

	Byte sbc[] = {0x11, 0x00, 0x00, 0x11, 0x01, 0x01, 0x32, 0x01,		//C = 1 (borrow)
				  0x11, 0x02, 0x10, 0x11, 0x03, 0x05, 0x33, 0x23, 0x01};	//R2 = 0x10 - 0x05 - 1
	RUN(m, sbc);
	check(m.cpu.getReg(2) == 0x0A, "SBC subtracts the borrow");

	Byte logic[] = {0x11, 0x00, 0xF0, 0x11, 0x01, 0x3C,
					0x10, 0x20, 0x10, 0x30, 0x10, 0x40,						//R2 = R3 = R4 = R0
					0x34, 0x21, 0x35, 0x31, 0x36, 0x41, 0x01};				//AND, OR, XOR with R1
	RUN(m, logic);
	check(m.cpu.getReg(2) == 0x30 && m.cpu.getReg(3) == 0xFC && m.cpu.getReg(4) == 0xCC, "MOV, AND, OR, XOR");
	check(flagsAre(m.cpu, 0, 0, 1, 0), "logic sets N, clears C and V");

	Byte cmp[] = {0x11, 0x00, 0x05, 0x11, 0x01, 0x07, 0x37, 0x01, 0x01};
	RUN(m, cmp);
	check(m.cpu.getReg(0) == 0x05 && m.cpu.getFlag(CPU::FLAG_C), "CMP keeps the register, sets borrow");

	Byte imm[] = {0x11, 0x00, 0x10, 0x38, 0x00, 0x05, 0x39, 0x00, 0x03, 0x3A, 0x00, 0x12, 0x01};
	RUN(m, imm);
	check(m.cpu.getReg(0) == 0x12 && m.cpu.getFlag(CPU::FLAG_Z), "ADDI, SUBI, CMPI");

	Byte inc[] = {0x11, 0x00, 0xFF, 0x11, 0x01, 0x01, 0x30, 0x01,		//C = 1
				  0x11, 0x02, 0x7F, 0x40, 0x02, 0x01};						//INC 0x7F
	RUN(m, inc);
	check(m.cpu.getReg(2) == 0x80 && flagsAre(m.cpu, 0, 1, 1, 1), "INC 0x7F = 0x80, V set, C unchanged");

	Byte dec[] = {0x11, 0x02, 0x80, 0x41, 0x02, 0x01};
	RUN(m, dec);
	check(m.cpu.getReg(2) == 0x7F && flagsAre(m.cpu, 0, 0, 0, 1), "DEC 0x80 = 0x7F, V set");

	Byte shifts[] = {0x11, 0x00, 0x81, 0x43, 0x00,							//SHL 0x81
					 0x11, 0x01, 0x01, 0x44, 0x01,							//SHR 0x01
					 0x11, 0x02, 0x0F, 0x42, 0x02, 0x01};					//NOT 0x0F
	RUN(m, shifts);
	check(m.cpu.getReg(0) == 0x02 && m.cpu.getReg(1) == 0x00 && m.cpu.getReg(2) == 0xF0, "SHL, SHR, NOT results");
	check(m.cpu.getFlag(CPU::FLAG_C), "SHR puts old bit 0 into C");

	Byte ldst[] = {0x11, 0x00, 0x2A, 0x13, 0x00, 0x00, 0x80,			//ST R0, [0x8000]
				   0x12, 0x01, 0x00, 0x80, 0x01};						//LD R1, [0x8000]
	RUN(m, ldst);
	check(m.ram.read(0) == 0x2A && m.cpu.getReg(1) == 0x2A, "ST then LD through the bus");

	Byte ptr[] = {0x11, 0x06, 0x80, 0x11, 0x07, 0xFF,						//P = 0x80FF
				  0x16,														//INP -> 0x8100
				  0x11, 0x00, 0x99, 0x15, 0x00,								//STP R0
				  0x17, 0x16, 0x14, 0x01, 0x01};							//DEP, INP, LDP R1
	RUN(m, ptr);
	check(m.cpu.getP() == 0x8100 && m.cpu.getReg(6) == 0x81 && m.cpu.getReg(7) == 0x00, "INP carries into R6");
	check(m.ram.read(0x100) == 0x99 && m.cpu.getReg(1) == 0x99, "STP / LDP use P");

	Byte vram[] = {0x11, 0x00, 0x01, 0x13, 0x00, 0x05, 0xE0, 0x01};		//ST R0, [0xE005]
	RUN(m, vram);
	check(m.vram.read(5) == 0x01, "a pixel written to 0xE005 lands in VRAM");
}

void testPrograms(){
	printf("--CPU programs--\n");
	Machine m;

	//sum 10 + 9 + ... + 1 into R0, store it at 0x8000
	Byte sum[] = {
		0x11, 0x00, 0x00,			//0x0000 LDI R0, 0
		0x11, 0x01, 0x0A,			//0x0003 LDI R1, 10
		0x30, 0x01,					//0x0006 ADD R0, R1
		0x41, 0x01,					//0x0008 DEC R1
		0x62, 0x06, 0x00,			//0x000A JNZ 0x0006
		0x13, 0x00, 0x00, 0x80,		//0x000D ST R0, [0x8000]
		0x01						//0x0011 HLT
	};
	StepResult r = RUN(m, sum);
	check(r == StepResult::Halt && m.cpu.getState() == CpuState::Halted, "loop program halts");
	check(m.cpu.getReg(0) == 55 && m.ram.read(0) == 55, "1 + 2 + ... + 10 = 55");
	check(m.cpu.getCycles() == 82, "cycle count is 82");
	check(m.cpu.getPC() == 0x0012, "PC stops after HLT");
	check(m.cpu.step() == StepResult::Halt && m.cpu.getCycles() == 82, "stepping a halted CPU does nothing");

	Byte call[] = {
		0x66, 0x07, 0x00,			//0x0000 CALL 0x0007
		0x11, 0x01, 0x02,			//0x0003 LDI R1, 2
		0x01,						//0x0006 HLT
		0x11, 0x00, 0x01,			//0x0007 LDI R0, 1
		0x67						//0x000A RET
	};
	m.rom.load(call, sizeof(call));
	m.cpu.reset();
	m.cpu.step();
	check(m.cpu.getPC() == 0x0007 && m.cpu.getSP() == 0xFFFD, "CALL jumps and pushes 2 bytes");
	check(m.stack.read(0x0EFF) == 0x00 && m.stack.read(0x0EFE) == 0x03, "return address 0x0003 on the stack, high byte first");
	m.cpu.run(100);
	check(m.cpu.getReg(0) == 1 && m.cpu.getReg(1) == 2 && m.cpu.getSP() == 0xFFFF, "RET comes back, stack empty again");

	Byte stack[] = {0x11, 0x00, 0x42, 0x80, 0x00, 0x81, 0x03,				//PUSH R0, POP R3
					0x11, 0x01, 0x00,										//Z = 1
					0x82, 0x11, 0x01, 0x05, 0x83, 0x01};					//PUSHF, LDI clears Z, POPF
	RUN(m, stack);
	check(m.cpu.getReg(3) == 0x42, "PUSH then POP moves a value");
	check(m.cpu.getFlag(CPU::FLAG_Z) && m.cpu.getSP() == 0xFFFF, "PUSHF / POPF restore the flags");

	Byte notTaken[] = {0x11, 0x00, 0x01, 0x61, 0x00, 0x90, 0x01};			//LDI R0,1 ; JZ 0x9000 ; HLT
	check(RUN(m, notTaken) == StepResult::Halt, "a jump that is not taken is not checked");

	//every conditional jump, with its flag on and off. The other flags are set the opposite way,
	//so a jump that looks at the wrong flag is caught too. Flags are loaded with PUSH R1 + POPF.
	struct JumpCase { Byte opcode; int bit; bool takeWhenSet; const char* name; };
	JumpCase jumpCases[] = {
		{0x61, 0, true,  "JZ"}, {0x62, 0, false, "JNZ"}, {0x63, 1, true, "JC"},
		{0x64, 1, false, "JNC"}, {0x65, 2, true, "JN"}
	};
	bool jumpsOk = true;
	for(int i = 0; i < 5; ++i){
		for(int flagOn = 0; flagOn <= 1; ++flagOn){
			Byte value = flagOn ? (Byte)(1 << jumpCases[i].bit) : (Byte)(0x0F & ~(1 << jumpCases[i].bit));
			Byte prog[] = {
				0x11, 0x01, value,			//0x0000 LDI R1, flags
				0x80, 0x01,					//0x0003 PUSH R1
				0x83,						//0x0005 POPF
				jumpCases[i].opcode, 0x10, 0x00,	//0x0006 Jxx 0x0010
				0x11, 0x00, 0x01,			//0x0009 LDI R0, 1   (not taken)
				0x01,						//0x000C HLT
				0x00, 0x00, 0x00,			//0x000D padding
				0x11, 0x00, 0x02,			//0x0010 LDI R0, 2   (taken)
				0x01						//0x0013 HLT
			};
			RUN(m, prog);
			bool shouldTake = (flagOn == 1) == jumpCases[i].takeWhenSet;
			if(m.cpu.getReg(0) != (shouldTake ? 2 : 1)){
				jumpsOk = false;
				printf("        %s wrong with its flag %s\n", jumpCases[i].name, flagOn ? "on" : "off");
			}
		}
	}
	check(jumpsOk, "JZ, JNZ, JC, JNC, JN each follow the right flag");

	Byte lastRomByte[] = {0x60, 0xFF, 0x7F};								//JMP 0x7FFF (last byte of ROM)
	m.rom.load(lastRomByte, sizeof(lastRomByte));
	m.cpu.reset();
	check(m.cpu.step() == StepResult::Ok && m.cpu.getPC() == 0x7FFF, "JMP to the last ROM byte is allowed");

	Byte popfMask[] = {0x11, 0x01, 0xFF, 0x80, 0x01, 0x83, 0x01};			//LDI R1,0xFF ; PUSH R1 ; POPF
	RUN(m, popfMask);
	check(m.cpu.getFlags() == 0x0F, "POPF keeps only the 4 flag bits");

	bool logicClears = true;
	for(Byte op = 0x34; op <= 0x36; ++op){									//AND, OR, XOR
		Byte prog[] = {0x11, 0x00, 0x80, 0x11, 0x01, 0x80, 0x30, 0x01,		//0x80 + 0x80 sets Z C V
					   0x11, 0x02, 0x01, op, 0x22, 0x01};					//R2 op R2 = 0x01 (XOR gives 0)
		RUN(m, prog);
		if(m.cpu.getFlag(CPU::FLAG_C) || m.cpu.getFlag(CPU::FLAG_V)) logicClears = false;
	}
	check(logicClears, "AND, OR, XOR clear C and V even when they were set");

	Byte loop[] = {0x60, 0x00, 0x00};										//JMP 0x0000 forever
	check(RUN(m, loop) == StepResult::Ok, "endless loop stops at the step limit");
	m.cpu.reset();
	check(m.cpu.run(50) == StepResult::Ok && m.cpu.getState() == CpuState::Running && m.cpu.getCycles() == 150, "run(50) runs exactly 50 steps");

	m.cpu.setReg(3, 0x77);
	m.cpu.reset();
	check(m.cpu.getReg(3) == 0 && m.cpu.getPC() == 0 && m.cpu.getSP() == 0xFFFF && m.cpu.getFlags() == 0
		&& m.cpu.getCycles() == 0 && m.cpu.getFault().type == FaultType::None, "reset clears everything");
	check(m.cpu.getReg(8) == 0 && m.cpu.getReg(-1) == 0, "getReg outside R0-R7 is safe");

	heardCount = 0;
	m.bus.setWriteListener([](Word address, Byte oldValue, Byte newValue){
		heardAddress = address; heardOld = oldValue; heardNew = newValue; heardCount++;
	});
	Byte undo[] = {0x11, 0x00, 0x2A, 0x13, 0x00, 0x10, 0x80, 0x01};		//ST R0, [0x8010]
	RUN(m, undo);
	check(heardCount == 1 && heardAddress == 0x8010 && heardOld == 0 && heardNew == 0x2A, "CPU writes reach the write listener (for undo)");
	m.bus.setWriteListener(WriteListener());
}

void testFaults(){
	printf("--CPU faults--\n");
	Machine m;

	Byte bad[] = {0x00, 0x00, 0xFF};
	check(RUN(m, bad) == StepResult::Fault, "invalid opcode stops the CPU");
	check(m.cpu.getFault().type == FaultType::InvalidOpcode && m.cpu.getFault().pc == 0x0002, "fault says InvalidOpcode at 0x0002");
	check(m.cpu.getState() == CpuState::Faulted && m.cpu.getPC() == 0x0002, "PC stays on the bad instruction");
	check(m.cpu.step() == StepResult::Fault, "a faulted CPU stays faulted");

	Byte badReg[] = {0x11, 0x08, 0x05};
	RUN(m, badReg);
	check(m.cpu.getFault().type == FaultType::InvalidRegister, "LDI R8 gives InvalidRegister");
	Byte badSrc[] = {0x30, 0x09};
	RUN(m, badSrc);
	check(m.cpu.getFault().type == FaultType::InvalidRegister, "ADD R0, R9 gives InvalidRegister");

	Byte romWrite[] = {0x00, 0x13, 0x00, 0x10, 0x00};						//NOP ; ST R0, [0x0010]
	RUN(m, romWrite);
	check(m.cpu.getFault().type == FaultType::IllegalWrite && m.cpu.getFault().address == 0x0010
		&& m.cpu.getFault().pc == 0x0001, "ST into ROM gives IllegalWrite with address and PC");
	check(m.cpu.getFault().message.find("ROM") != std::string::npos, "message names the chip");
	check(!m.bus.hasFault(), "the CPU takes the fault off the bus");

	Byte unmapped[] = {0x12, 0x00, 0x00, 0xF0};								//LD R0, [0xF000] (no I/O chip attached here)
	RUN(m, unmapped);
	check(m.cpu.getFault().type == FaultType::UnmappedAccess && m.cpu.getFault().address == 0xF000, "LD from an empty address gives UnmappedAccess");

	Byte popEmpty[] = {0x81, 0x00};
	RUN(m, popEmpty);
	check(m.cpu.getFault().type == FaultType::StackUnderflow, "POP on empty stack gives StackUnderflow");
	Byte retEmpty[] = {0x67};
	RUN(m, retEmpty);
	check(m.cpu.getFault().type == FaultType::StackUnderflow, "RET on empty stack gives StackUnderflow");

	Byte pushForever[] = {0x80, 0x00, 0x60, 0x00, 0x00};					//PUSH R0 ; JMP 0x0000
	RUN(m, pushForever);
	check(m.cpu.getFault().type == FaultType::StackOverflow && m.cpu.getSP() == 0xF0FF, "3840 pushes fill the stack, the next one overflows");

	Byte badJump[] = {0x60, 0x00, 0x80};									//JMP 0x8000
	RUN(m, badJump);
	check(m.cpu.getFault().type == FaultType::InvalidJumpTarget && m.cpu.getFault().address == 0x8000, "JMP into RAM gives InvalidJumpTarget");
	Byte badCall[] = {0x66, 0x00, 0xE0};
	RUN(m, badCall);
	check(m.cpu.getFault().type == FaultType::InvalidJumpTarget && m.cpu.getSP() == 0xFFFF, "bad CALL is refused before pushing");

	printf("        example: %s - %s\n", faultName(m.cpu.getFault().type), m.cpu.getFault().message.c_str());

	Byte nop[] = {0x00, 0x01};
	m.rom.load(nop, sizeof(nop));
	m.cpu.reset();
	m.bus.read(0xF000);															//someone else leaves a fault on the bus
	check(m.cpu.step() == StepResult::Ok, "an old fault left on the bus is not blamed on the CPU");
}

void testDisassembler(){
	printf("--Disassembler--\n");
	Machine m;
	Byte program[] = {
		0x00,						//0x0000 NOP
		0x40, 0x01,					//0x0001 INC R1
		0x30, 0x25,					//0x0003 ADD R2, R5
		0x11, 0x00, 0x2A,			//0x0005 LDI R0, 0x2A
		0x60, 0x10, 0x00,			//0x0008 JMP 0x0010
		0x12, 0x00, 0x00, 0x80,		//0x000B LD R0, [0x8000]
		0x14, 0x03,					//0x000F LDP R3
		0xFF						//0x0011 bad byte
	};
	m.rom.load(program, sizeof(program));
	Disassembler dis(m.bus);

	const char* expected[] = {
		"0x0000  NOP", "0x0001  INC R1", "0x0003  ADD R2, R5", "0x0005  LDI R0, 0x2A",
		"0x0008  JMP 0x0010", "0x000B  LD R0, [0x8000]", "0x000F  LDP R3", "0x0011  DB 0xFF"
	};
	Word address = 0;
	bool allMatch = true;
	for(int i = 0; i < 8; ++i){
		Byte length = 0;
		std::string text = dis.line(address, length);
		printf("        %s\n", text.c_str());
		if(text != expected[i]) allMatch = false;
		address = (Word)(address + length);
	}
	check(allMatch, "every format prints correctly and lengths walk the program");

	Byte length = 0;
	check(dis.line(0xF000, length) == "0xF000  DB 0xFF" && length == 1, "empty address prints as DB 0xFF");
	check(!m.bus.hasFault(), "disassembling never leaves a fault on the bus");
}

int main(){
	testBits();
	testMemoryMap();
	testFault();
	testRamRom();
	testBus();
	testOpcodes();
	testFlagTable();
	testInstructions();
	testPrograms();
	testFaults();
	testDisassembler();

	printf("\n%d passed, %d failed\n", passed, failed);
	return failed == 0 ? 0 : 1;
}
