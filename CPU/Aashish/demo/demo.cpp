//LIVE DEMO OF THE PIXEL-8 CPU AND MEMORY
//run: mingw32-make demo        (press Enter to go to the next part)
//     demo.exe nopause         (runs everything without stopping)

#include <cstdio>
#include <cstring>
#include <iostream>
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
#include "../src/IoPorts.h"
#include "../src/Console.h"

bool interactive = true;
bool showWrites = false;

//the memory chips wired like MemoryMap.h (no I/O ports: Part 7 uses the real Console class for those)
struct Board{
	Rom rom;
	Ram ram;
	Ram vram;
	Ram stack;
	Bus bus;
	CPU cpu;
	Disassembler dis;

	Board() : rom(0x8000), ram(0x6000, "RAM"), vram(0x1000, "VRAM"), stack(0x0F00, "STACK"), bus(), cpu(bus), dis(bus){
		bus.attach(MemoryMap::ROM_START,   MemoryMap::ROM_END,   &rom);
		bus.attach(MemoryMap::RAM_START,   MemoryMap::RAM_END,   &ram);
		bus.attach(MemoryMap::VRAM_START,  MemoryMap::VRAM_END,  &vram);
		bus.attach(MemoryMap::STACK_START, MemoryMap::STACK_END, &stack);

		//the hook the State Manager will use for undo; here it just prints every write
		bus.setWriteListener([](Word address, Byte oldValue, Byte newValue){
			if(showWrites) printf("%32s bus write [0x%04X]: 0x%02X -> 0x%02X\n", "", address, oldValue, newValue);
		});
	}

	void load(const Byte* program, unsigned int length){
		rom.load(program, length);
		cpu.reset();
	}
};

void pause(){
	if(!interactive) return;
	printf("\n   [ press Enter to continue ]");
	fflush(stdout);
	int c;
	while((c = getchar()) != '\n' && c != EOF) {}
}

void title(const char* text){
	printf("\n================================================================================\n");
	printf("  %s\n", text);
	printf("================================================================================\n");
}

std::string flagText(const CPU& cpu){
	std::string s;
	s += cpu.getFlag(CPU::FLAG_Z) ? "Z" : "-";
	s += cpu.getFlag(CPU::FLAG_C) ? "C" : "-";
	s += cpu.getFlag(CPU::FLAG_N) ? "N" : "-";
	s += cpu.getFlag(CPU::FLAG_V) ? "V" : "-";
	return s;
}

void traceHeader(){
	printf("  %-28s %-20s%-6s%-8s%s\n", "instruction", "registers after", "flags", "SP", "cycles");
	printf("  %-28s %-20s%-6s%-8s%s\n", "-----------", "---------------", "-----", "----", "------");
}

//disassemble the next instruction, run it, print the machine state after it
StepResult traceStep(Board& c){
	Byte length = 0;
	std::string text = c.dis.line(c.cpu.getPC(), length);
	StepResult r = c.cpu.step();
	printf("  %-28s R0=%02X R1=%02X R2=%02X   %s  %04X    %lu\n", text.c_str(),
		c.cpu.getReg(0), c.cpu.getReg(1), c.cpu.getReg(2), flagText(c.cpu).c_str(), c.cpu.getSP(), c.cpu.getCycles());
	return r;
}

void listing(Board& c, Word from, Word to){
	Word address = from;
	while(address < to){
		Byte length = 0;
		printf("    %s\n", c.dis.line(address, length).c_str());
		address = (Word)(address + length);
	}
}

//------------------------------------------------------------------------------------------------

void partMemory(Board& c){
	title("PART 1  MEMORY MAP AND BUS: every address goes to the right chip");

	printf("  Address range    Chip    Size        Used for\n");
	printf("  0x0000-0x7FFF    ROM     %5u B     the program (read only)\n", c.rom.size());
	printf("  0x8000-0xDFFF    RAM     %5u B     variables\n", c.ram.size());
	printf("  0xE000-0xEFFF    VRAM    %5u B     screen pixels (read by the PPU)\n", c.vram.size());
	printf("  0xF000-0xF0FF    I/O       256 B     ports (PPU side, not attached in this demo)\n");
	printf("  0xF100-0xFFFF    STACK   %5u B     grows down from 0xFFFF\n\n", c.stack.size());

	c.bus.write(0x8005, 0x42);
	printf("  bus.write(0x8005, 0x42)  -> RAM offset 5 now holds 0x%02X  (0x8005 - 0x8000 = 5)\n", c.ram.read(5));
	c.bus.write(0xE001, 0x07);
	printf("  bus.write(0xE001, 0x07)  -> VRAM offset 1 now holds 0x%02X\n", c.vram.read(1));
	printf("  bus.readWord(0x8005) after writing 0x1234 there -> ");
	c.bus.writeWord(0x8005, 0x1234);
	printf("0x%04X (stored low byte first: 0x%02X 0x%02X)\n\n", c.bus.readWord(0x8005), c.ram.read(5), c.ram.read(6));

	printf("  Mistakes are caught, not crashed on:\n");
	c.bus.write(0x0003, 0x99);
	printf("  bus.write(0x0003, 0x99)  -> %-15s %s\n", faultName(c.bus.getFault().type), c.bus.getFault().message.c_str());
	c.bus.clearFault();
	Byte v = c.bus.read(0xF000);
	printf("  bus.read(0xF000)         -> %-15s %s (returned 0x%02X)\n", faultName(c.bus.getFault().type), c.bus.getFault().message.c_str(), v);
	c.bus.clearFault();
	Ram extra(0x100, "EXTRA");
	printf("  attach a chip over RAM   -> %s\n", c.bus.attach(0x8000, 0x80FF, &extra) ? "accepted" : "refused (overlaps RAM)");

	c.ram.clear();
	c.vram.clear();
	pause();
}

void partTrace(Board& c){
	title("PART 2  FETCH - DECODE - EXECUTE, one instruction at a time");

	Byte program[] = {
		0x11, 0x00, 0x00,			//LDI R0, 0
		0x11, 0x01, 0x0A,			//LDI R1, 10
		0x30, 0x01,					//ADD R0, R1      <- loop
		0x41, 0x01,					//DEC R1
		0x62, 0x06, 0x00,			//JNZ 0x0006      -> back to loop while R1 != 0
		0x13, 0x00, 0x00, 0x80,		//ST R0, [0x8000]
		0x01						//HLT
	};
	c.load(program, sizeof(program));

	printf("  Program: add 10 + 9 + ... + 1 into R0, then store it in RAM at 0x8000.\n");
	printf("  These are the raw bytes in ROM, turned back into text by the Disassembler:\n\n");
	listing(c, 0x0000, sizeof(program));
	pause();

	printf("\n");
	traceHeader();
	showWrites = true;
	while(traceStep(c) == StepResult::Ok) {}
	showWrites = false;

	printf("\n  Result: R0 = %d, RAM[0x8000] = %d, %lu clock cycles, CPU state = %s\n",
		c.cpu.getReg(0), c.ram.read(0), c.cpu.getCycles(),
		c.cpu.getState() == CpuState::Halted ? "Halted" : "not halted");
	pause();
}

void partFlags(Board& c){
	title("PART 3  FLAGS: Z (zero)  C (carry/borrow)  N (negative)  V (signed overflow)");

	struct Row { Byte a; Byte b; Byte op; const char* sign; const char* expect; const char* why; };
	Row rows[] = {
		{0xFF, 0x01, 0x30, "+", "ZC--", "255 + 1 = 256, does not fit, wraps to 0"},
		{0x7F, 0x01, 0x30, "+", "--NV", "127 + 1 = 128, but signed 0x80 is -128"},
		{0x05, 0x05, 0x32, "-", "Z---", "equal numbers"},
		{0x00, 0x01, 0x32, "-", "-CN-", "had to borrow"},
		{0x80, 0x01, 0x32, "-", "---V", "-128 - 1 = -129, does not fit signed"}
	};

	printf("  Calculation       Result  Flags  Booklet  Why\n");
	for(int i = 0; i < 5; ++i){
		Byte program[] = {0x11, 0x00, rows[i].a, 0x11, 0x01, rows[i].b, rows[i].op, 0x01, 0x01};
		c.load(program, sizeof(program));
		c.cpu.run(10);
		std::string got = flagText(c.cpu);
		printf("  0x%02X %s 0x%02X       0x%02X    %s   %s %s  %s\n", rows[i].a, rows[i].sign, rows[i].b,
			c.cpu.getReg(0), got.c_str(), rows[i].expect, got == rows[i].expect ? "OK" : "!!", rows[i].why);
	}
	pause();
}

void partStack(Board& c){
	title("PART 4  STACK, CALL AND RET: a subroutine call");

	Byte program[] = {
		0x11, 0x00, 0x05,			//0x0000 LDI R0, 5
		0x66, 0x0A, 0x00,			//0x0003 CALL 0x000A    (subroutine: double R0)
		0x11, 0x02, 0x01,			//0x0006 LDI R2, 1       <- RET comes back here
		0x01,						//0x0009 HLT
		0x80, 0x00,					//0x000A PUSH R0         subroutine starts
		0x81, 0x01,					//0x000C POP R1
		0x30, 0x01,					//0x000E ADD R0, R1
		0x67						//0x0010 RET
	};
	c.load(program, sizeof(program));

	printf("  Program:\n");
	listing(c, 0x0000, sizeof(program));
	printf("\n  Stack starts empty at SP = 0xFFFF and grows DOWN. CALL pushes the return\n");
	printf("  address 0x0006 (high byte, then low byte); RET pops it back into PC.\n\n");
	traceHeader();
	showWrites = true;
	StepResult r;
	do{
		r = traceStep(c);
		if(c.cpu.getSP() != 0xFFFF){
			printf("%32s stack: ", "");
			for(Word a = 0xFFFF; a > c.cpu.getSP(); --a) printf("[0x%04X]=0x%02X ", a, c.bus.read(a));
			printf("\n");
		}
	} while(r == StepResult::Ok);
	showWrites = false;

	printf("\n  Result: R0 = %d (5 doubled), back in the main program (R2 = %d), SP = 0x%04X (empty again)\n",
		c.cpu.getReg(0), c.cpu.getReg(2), c.cpu.getSP());
	pause();
}

void partFaults(Board& c){
	title("PART 5  FAULT HANDLING: broken programs are stopped and reported, never crash");

	struct Case { const char* what; Byte bytes[8]; unsigned int length; };
	Case cases[] = {
		{"byte 0xFF is not an instruction",   {0x00, 0xFF},                   2},
		{"LDI R9, 1  (only R0-R7 exist)",     {0x11, 0x09, 0x01},             3},
		{"ST R0, [0x0010]  (write into ROM)", {0x13, 0x00, 0x10, 0x00},       4},
		{"LD R0, [0xF000]  (no chip there)",  {0x12, 0x00, 0x00, 0xF0},       4},
		{"POP R0 on an empty stack",          {0x81, 0x00},                   2},
		{"JMP 0x9000  (outside the program)", {0x60, 0x00, 0x90},             3},
		{"PUSH R0 forever",                   {0x80, 0x00, 0x60, 0x00, 0x00}, 5}
	};

	printf("  %-36s %-18s %-8s %s\n", "Broken program", "Fault", "at PC", "Message");
	for(int i = 0; i < 7; ++i){
		c.load(cases[i].bytes, cases[i].length);
		c.cpu.run(100000);
		const Fault& f = c.cpu.getFault();
		printf("  %-36s %-18s 0x%04X   %s\n", cases[i].what, faultName(f.type), f.pc, f.message.c_str());
	}
	printf("\n  (\"PUSH R0 forever\" filled all %u stack bytes first; SP stopped at 0x%04X.)\n", c.stack.size(), c.cpu.getSP());
	printf("  After a fault the CPU stays stopped until reset(), and PC points at the bad instruction.\n");

	//the CPU's instruction trace (a circular queue of the last 16) answers "how did it get there?"
	const InstructionTrace& trace = c.cpu.getTrace();
	printf("\n  The last 4 of the %d instructions the CPU keeps in its trace (newest = the one that failed):\n", trace.size());
	for(int i = (trace.size() > 4 ? trace.size() - 4 : 0); i < trace.size(); ++i){
		Byte length = 0;
		printf("    %s\n", c.dis.line(trace.at(i).pc, length).c_str());
	}
	std::cout << "\n  std::cout << cpu:  " << c.cpu << "\n";
	pause();
}

void partScreen(Board& c){
	title("PART 6  GRAPHICS: a CPU program draws into VRAM (0xE000), shown as a 32x16 screen");

	//pixel (x, y) lives at 0xE000 + y*32 + x. P = R6:R7 is the 16-bit pointer.
	Byte program[] = {
		0x11, 0x00, 0x01,			//0x0000 LDI R0, 1        pixel colour
		0x11, 0x05, 0x00,			//0x0003 LDI R5, 0        zero, for ADC
		0x11, 0x06, 0xE0,			//0x0006 LDI R6, 0xE0     P = 0xE000 (top row)
		0x11, 0x07, 0x00,			//0x0009 LDI R7, 0x00
		0x11, 0x01, 0x20,			//0x000C LDI R1, 32
		0x15, 0x00,					//0x000F STP R0           top row loop
		0x16,						//0x0011 INP
		0x41, 0x01,					//0x0012 DEC R1
		0x62, 0x0F, 0x00,			//0x0014 JNZ 0x000F
		0x11, 0x06, 0xE1,			//0x0017 LDI R6, 0xE1     P = 0xE1E0 (bottom row)
		0x11, 0x07, 0xE0,			//0x001A LDI R7, 0xE0
		0x11, 0x01, 0x20,			//0x001D LDI R1, 32
		0x15, 0x00,					//0x0020 STP R0           bottom row loop
		0x16,						//0x0022 INP
		0x41, 0x01,					//0x0023 DEC R1
		0x62, 0x20, 0x00,			//0x0025 JNZ 0x0020
		0x11, 0x06, 0xE0,			//0x0028 LDI R6, 0xE0     P = 0xE000 (top-left)
		0x11, 0x07, 0x00,			//0x002B LDI R7, 0x00
		0x11, 0x01, 0x10,			//0x002E LDI R1, 16
		0x15, 0x00,					//0x0031 STP R0           diagonal loop: P += 33 (down-right)
		0x38, 0x07, 0x21,			//0x0033 ADDI R7, 33
		0x31, 0x65,					//0x0036 ADC R6, R5       carry into the high byte
		0x41, 0x01,					//0x0038 DEC R1
		0x62, 0x31, 0x00,			//0x003A JNZ 0x0031
		0x11, 0x06, 0xE0,			//0x003D LDI R6, 0xE0     P = 0xE01F (top-right)
		0x11, 0x07, 0x1F,			//0x0040 LDI R7, 0x1F
		0x11, 0x01, 0x10,			//0x0043 LDI R1, 16
		0x15, 0x00,					//0x0046 STP R0           diagonal loop: P += 31 (down-left)
		0x38, 0x07, 0x1F,			//0x0048 ADDI R7, 31
		0x31, 0x65,					//0x004B ADC R6, R5
		0x41, 0x01,					//0x004D DEC R1
		0x62, 0x46, 0x00,			//0x004F JNZ 0x0046
		0x01						//0x0052 HLT
	};
	c.load(program, sizeof(program));
	c.vram.clear();

	printf("  Program (draws two lines and two diagonals with loops and 16-bit pointer maths):\n");
	listing(c, 0x0000, sizeof(program));
	pause();

	unsigned long steps = 0;
	while(c.cpu.step() == StepResult::Ok) steps++;
	steps++;		//the HLT

	printf("\n  The CPU ran %lu instructions in %lu cycles. This is what is now in VRAM:\n\n", steps, c.cpu.getCycles());
	for(int y = 0; y < 16; ++y){
		printf("    ");
		for(int x = 0; x < 32; ++x) printf("%s", c.vram.read((Word)(y * 32 + x)) ? "##" : ". ");
		printf("\n");
	}
	printf("\n  Ashmit's PPU will read these same VRAM bytes to draw the real screen.\n");
	pause();
}

void partConnections(){
	title("PART 7  CONNECTIONS: the full Console with I/O ports (0xF000) for the other units");

	Console con;
	printf("  Console = ROM + RAM + VRAM + IO + STACK on one Bus. Every one of the 65536 addresses has a chip.\n");
	printf("    0xF000 INPUT   buttons     0xF001 RNG     random byte    0xF002 PPU_CTRL\n");
	printf("    0xF003 FRAME   counter     0xF004 CONSOLE_OUT  text\n");

	//1. text out: the console listener prints each character the moment the CPU writes it
	Byte hello[] = {
		0x11, 0x00, 0x48, 0x13, 0x00, 0x04, 0xF0,		//LDI R0, 'H' ; ST R0, [0xF004]
		0x11, 0x00, 0x49, 0x13, 0x00, 0x04, 0xF0,		//LDI R0, 'I' ; ST R0, [0xF004]
		0x01											//HLT
	};
	con.io().setConsoleListener([](char ch){ printf("%c", ch); });
	printf("\n  1. A program writes to CONSOLE_OUT. It prints: ");
	con.loadProgram(hello, sizeof(hello));
	con.runFrame();
	printf("\n");
	con.io().setConsoleListener(nullptr);

	//2. input: the keyboard side holds buttons, the program reads them as bits
	Byte readInput[] = {0x12, 0x00, 0x00, 0xF0, 0x01};	//LD R0, [0xF000] ; HLT
	con.loadProgram(readInput, sizeof(readInput));
	con.io().setButton(IoPorts::BTN_RIGHT, true);
	con.io().setButton(IoPorts::BTN_A, true);
	con.runFrame();
	printf("\n  2. RIGHT (bit 3) and A (bit 4) are held. LD R0, [0xF000] gives R0 = 0x%02X\n", con.cpu().getReg(0));

	//3. frames + random numbers: the program waits for each new frame, then takes a random byte
	Byte game[] = {
		0x12, 0x01, 0x03, 0xF0,		//0x0000 LD R1, [0xF003]   frame now
		0x12, 0x00, 0x03, 0xF0,		//0x0004 LD R0, [0xF003]   wait:
		0x37, 0x01,					//0x0008 CMP R0, R1
		0x61, 0x04, 0x00,			//0x000A JZ 0x0004         same frame: keep waiting
		0x12, 0x02, 0x01, 0xF0,		//0x000D LD R2, [0xF001]   random byte
		0x13, 0x02, 0x00, 0x80,		//0x0011 ST R2, [0x8000]   save it in RAM
		0x60, 0x00, 0x00			//0x0015 JMP 0x0000
	};
	con.loadProgram(game, sizeof(game));
	printf("\n  3. A game loop that runs once per frame (waits for FRAME to change, then reads RNG):\n");
	con.runFrame();
	printf("     frame 1: the program only waits, nothing saved yet\n");
	for(int i = 1; i < 6; ++i){
		con.runFrame();
		printf("     after frame %d: FRAME port = %d, random byte saved at 0x8000 = 0x%02X\n",
			i + 1, con.io().getFrame(), con.bus().peek(0x8000));
	}
	printf("\n  Arnav: loadProgram(assembler bytes) + runFrame()   Ashmit: pixel(x, y) + io().setButton()\n");
	printf("  Piyush: bus().setWriteListener() to record, bus().poke() + cpu().setPC()... to undo\n");
	pause();
}

int main(int argc, char** argv){
	if(argc > 1 && std::strcmp(argv[1], "nopause") == 0) interactive = false;

	printf("\n  PIXEL-8 CPU AND MEMORY DEMO  (Aashish Singh, CPU architecture and memory)\n");
	printf("  38 instructions, 8 registers, 64 KB memory map, ROM/RAM/VRAM/IO/STACK on one Bus\n");
	pause();

	Board c;
	partMemory(c);
	partTrace(c);
	partFlags(c);
	partStack(c);
	partFaults(c);
	partScreen(c);
	partConnections();

	printf("\n  End of demo. The 158 automatic tests: mingw32-make\n\n");
	return 0;
}
