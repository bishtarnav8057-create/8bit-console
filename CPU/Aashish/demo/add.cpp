//ONE ADDITION, STEP BY STEP: THE SIMPLEST PIXEL-8 DEMO
//run: mingw32-make add              (your own numbers: mingw32-make add A=25 B=17)
//a 5-instruction program adds two numbers. One picture of Memory, Bus and CPU stays on screen,
//and every key press shows one phase of one instruction: FETCH, DECODE or EXECUTE.
//the parts the numbers pass through light up one after another.
//
//honest about what is real: cpu.step() does fetch+decode+execute in one call. The demo shows
//FETCH with the same bytes the CPU will read (bus.peek at the PC), DECODE with the same table the
//CPU uses (OpcodeTable::lookup), and EXECUTE by really calling cpu.step(). Registers, PC, RAM and
//the carry flag on screen are read back from the emulator, never typed in.

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <string>
#include <vector>

#include "../src/Bits.h"
#include "../src/MemoryMap.h"
#include "../src/Ram.h"
#include "../src/Rom.h"
#include "../src/Bus.h"
#include "../src/Opcodes.h"
#include "../src/CPU.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <conio.h>
#else
#include <unistd.h>
#endif

//------------------------------------------------------------------------------------------------
//TERMINAL: colours, keys, drawing one whole screen at a time (same helpers as live.cpp)

const int SCREEN_W = 79;

const std::string RST    = "\x1b[0m";
const std::string BOLD   = "\x1b[1m";
const std::string GRAY   = "\x1b[90m";
const std::string CYAN   = "\x1b[96m";
const std::string YEL    = "\x1b[93m";
const std::string GRN    = "\x1b[92m";
const std::string RED    = "\x1b[91m";
const std::string ON_GRN = "\x1b[30;102m";
const std::string ON_YEL = "\x1b[30;103m";
const std::string ON_CYN = "\x1b[30;106m";

enum Key { K_NONE = 0, K_ENTER = 13, K_BACK = 8, K_ESC = 27, K_LEFT = 1000, K_RIGHT, K_EOF };

bool keyboard = false;		//true when keys come from a real console, false when piped in (tests)

void restoreTerminal(){
	printf("\x1b[0m\x1b[?25h\x1b[?1049l");
	fflush(stdout);
}

#ifdef _WIN32
BOOL WINAPI onCtrl(DWORD){
	restoreTerminal();
	return FALSE;
}
#endif

bool setupTerminal(){
#ifdef _WIN32
	DWORD mode = 0;
	keyboard = GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode) != 0;
	SetConsoleOutputCP(65001);
	HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
	if(GetConsoleMode(out, &mode)){
		if(!SetConsoleMode(out, mode | 0x0004)) return false;	//ENABLE_VIRTUAL_TERMINAL_PROCESSING
	}
	SetConsoleCtrlHandler(onCtrl, TRUE);
#endif
	printf("\x1b[?1049h\x1b[?25l");
	atexit(restoreTerminal);
	return true;
}

int terminalWidth(){
#ifdef _WIN32
	CONSOLE_SCREEN_BUFFER_INFO info;
	if(GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info))
		return info.srWindow.Right - info.srWindow.Left + 1;
#endif
	return SCREEN_W + 1;
}

int getKey(){
	int c;
#ifdef _WIN32
	if(keyboard){
		c = _getch();
		if(c == 0 || c == 224){
			switch(_getch()){
				case 75: return K_LEFT;
				case 77: return K_RIGHT;
			}
			return K_NONE;
		}
		if(c == 3) return K_EOF;				//Ctrl+C
		return c;
	}
#endif
	c = getchar();
	while(c == '\r') c = getchar();
	if(c == EOF) return K_EOF;
	if(c == '\n') return K_ENTER;
	return c;
}

bool keyWaiting(){
#ifdef _WIN32
	if(keyboard) return _kbhit() != 0;
#endif
	return false;
}

void sleepMs(int ms){
	if(!keyboard) return;						//piped input: no animation
#ifdef _WIN32
	Sleep(ms);
#else
	usleep(ms * 1000);
#endif
}

std::string fmt(const char* format, ...){
	char text[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(text, sizeof(text), format, args);
	va_end(args);
	return text;
}

int vlen(const std::string& s){
	int n = 0;
	for(size_t i = 0; i < s.size(); ++i){
		unsigned char c = (unsigned char)s[i];
		if(c == 0x1b){
			while(i < s.size() && s[i] != 'm') ++i;
			continue;
		}
		if((c & 0xC0) != 0x80) ++n;
	}
	return n;
}

std::string pad(const std::string& s, int width){
	int n = vlen(s);
	return n >= width ? s : s + std::string(width - n, ' ');
}

std::string rep(const std::string& piece, int count){
	std::string s;
	for(int i = 0; i < count; ++i) s += piece;
	return s;
}

typedef std::vector<std::string> Lines;

void show(const Lines& lines){
	std::string out = "\x1b[H";
	for(size_t i = 0; i < lines.size(); ++i) out += lines[i] + RST + "\x1b[K\n";
	out += "\x1b[J";
	fwrite(out.data(), 1, out.size(), stdout);
	fflush(stdout);
}

//a panel with its title in the top border, one space inside each side
Lines box(const std::string& title, int inner, const Lines& body, int height){
	int width = inner + 4;
	Lines out;
	out.push_back(GRAY + "┌─ " + RST + BOLD + CYAN + title + RST + GRAY + " " + rep("─", width - 5 - vlen(title)) + "┐" + RST);
	for(int i = 0; i < height; ++i){
		std::string line = i < (int)body.size() ? body[i] : "";
		out.push_back(GRAY + "│" + RST + " " + pad(line, inner) + RST + " " + GRAY + "│" + RST);
	}
	out.push_back(GRAY + "└" + rep("─", width - 2) + "┘" + RST);
	return out;
}

Lines wrap(const std::string& text, int width){
	Lines out;
	std::string line, word;
	for(size_t i = 0; i <= text.size(); ++i){
		if(i < text.size() && text[i] != ' '){ word += text[i]; continue; }
		if(!line.empty() && vlen(line) + 1 + vlen(word) > width){ out.push_back(line); line = ""; }
		line += (line.empty() ? "" : " ") + word;
		word = "";
	}
	if(!line.empty()) out.push_back(line);
	return out;
}

std::string hexBytes(const std::vector<Byte>& bytes){
	std::string s;
	for(size_t i = 0; i < bytes.size(); ++i) s += fmt(i ? " %02X" : "%02X", bytes[i]);
	return s;
}

//------------------------------------------------------------------------------------------------
//THE MACHINE AND THE PROGRAM

const Word ANSWER = 0x8000;			//RAM address the answer is stored at

struct Machine{
	Rom rom;
	Ram ram;
	Ram stack;
	Bus bus;
	CPU cpu;

	explicit Machine(const std::vector<Byte>& program)
		: rom(0x8000), ram(0x6000, "RAM"), stack(0x0F00, "STACK"), bus(), cpu(bus){
		bus.attach(MemoryMap::ROM_START,   MemoryMap::ROM_END,   &rom);
		bus.attach(MemoryMap::RAM_START,   MemoryMap::RAM_END,   &ram);
		bus.attach(MemoryMap::STACK_START, MemoryMap::STACK_END, &stack);
		rom.load(program.data(), (unsigned int)program.size());
		cpu.reset();
	}
};

//  LDI R0, a      put a into R0
//  LDI R1, b      put b into R1
//  ADD R0, R1     R0 = R0 + R1
//  ST  R0, [8000] save R0 in RAM
//  HLT            stop
std::vector<Byte> makeProgram(Byte a, Byte b){
	Byte program[] = { OP_LDI, 0, a,  OP_LDI, 1, b,  OP_ADD, 0x01,
	                   OP_ST, 0, lowByte(ANSWER), highByte(ANSWER),  OP_HLT };
	return std::vector<Byte>(program, program + sizeof(program));
}

struct Instr{
	Word addr;
	std::vector<Byte> bytes;
};

//split the program into instructions, using the opcode table's lengths
std::vector<Instr> splitProgram(const std::vector<Byte>& program){
	std::vector<Instr> out;
	for(size_t at = 0; at < program.size(); ){
		Byte length = OpcodeTable::lookup(program[at]).length;
		Instr in;
		in.addr = (Word)at;
		in.bytes.assign(program.begin() + at, program.begin() + at + length);
		out.push_back(in);
		at += length;
	}
	return out;
}

//the instruction written the way a programmer would, and what it means in plain words
std::string asmText(const std::vector<Byte>& b){
	switch(b[0]){
		case OP_LDI: return fmt("LDI R%d, %d", b[1], b[2]);
		case OP_ADD: return fmt("ADD R%d, R%d", b[1] >> 4, b[1] & 0x0F);
		case OP_ST:  return fmt("ST  R%d, [%04X]", b[1], makeWord(b[2], b[3]));
		case OP_HLT: return "HLT";
	}
	return "?";
}

std::string meaning(const std::vector<Byte>& b){
	switch(b[0]){
		case OP_LDI: return fmt("put %d in R%d", b[2], b[1]);
		case OP_ADD: return fmt("R%d = R%d + R%d", b[1] >> 4, b[1] >> 4, b[1] & 0x0F);
		case OP_ST:  return fmt("save R%d in RAM %04X", b[1], makeWord(b[2], b[3]));
		case OP_HLT: return "stop the program";
	}
	return "?";
}

//------------------------------------------------------------------------------------------------
//ONE SCREEN'S WORTH OF FACTS, worked out by running a fresh machine up to that point

enum Phase { FETCH = 0, DECODE = 1, EXECUTE = 2 };

struct View{
	Byte a, b;
	std::vector<Instr> prog;
	int   instr;				//which instruction, 0..4
	Phase phase;
	std::vector<Byte> fetched;	//bytes read from the bus at the PC
	const OpcodeInfo* info;
	Byte  regBefore[2], regAfter[2];
	Word  pcBefore, pcAfter;
	Byte  ramBefore, ramAfter;
	bool  carry;
	unsigned long cycles;
};

View buildView(Byte a, Byte b, int step){		//step 1..3*N
	View v;
	v.a = a;
	v.b = b;
	std::vector<Byte> program = makeProgram(a, b);
	v.prog  = splitProgram(program);
	v.instr = (step - 1) / 3;
	v.phase = (Phase)((step - 1) % 3);

	Machine m(program);
	for(int i = 0; i < v.instr; ++i) m.cpu.step();

	Word pc = m.cpu.getPC();
	v.info = &OpcodeTable::lookup(m.bus.peek(pc));
	for(int k = 0; k < v.info->length; ++k) v.fetched.push_back(m.bus.peek((Word)(pc + k)));

	v.pcBefore  = pc;
	v.ramBefore = m.bus.peek(ANSWER);
	for(int r = 0; r < 2; ++r) v.regBefore[r] = m.cpu.getReg(r);

	if(v.phase == EXECUTE) m.cpu.step();

	v.pcAfter  = m.cpu.getPC();
	v.ramAfter = m.bus.peek(ANSWER);
	for(int r = 0; r < 2; ++r) v.regAfter[r] = m.cpu.getReg(r);
	v.carry  = m.cpu.getFlag(CPU::FLAG_C);
	v.cycles = m.cpu.getCycles();
	return v;
}

//how many animation ticks a phase has: each tick lights the next part of the path
int lastTick(const View& v){
	if(v.phase == FETCH)  return 3;
	if(v.phase == DECODE) return 2;
	switch(v.fetched[0]){
		case OP_LDI: return 2;
		case OP_ADD: return 3;
		case OP_ST:  return 2;
	}
	return 0;
}

//------------------------------------------------------------------------------------------------
//DRAWING THE DIAGRAM

const int MEM_INNER = 19;
const int BUS_W     = 16;
const int CPU_INNER = 36;
const int BODY_H    = 10;

//rows inside the boxes (0 = first line under the top border)
const int ROW_IR   = 1;
const int ROW_REGS = 5;
const int ROW_RAM  = 8;
int romRow(int instr){ return 1 + instr; }

std::string hi(const std::string& s){ return ON_YEL + BOLD + s + RST; }	//lit up: data is here now
std::string label(const std::string& s, bool lit){ return lit ? YEL + BOLD + s + RST : s; }

std::string phaseStrip(const View& v){
	const char* names[] = { "FETCH", "DECODE", "EXECUTE" };
	const char* hints[] = { "get it", "understand it", "do it" };
	std::string s;
	for(int p = 0; p < 3; ++p){
		std::string name = fmt(" %s ", names[p]);
		if(p == v.phase)     s += ON_YEL + BOLD + name + RST;
		else if(p < v.phase) s += GRN + name + RST;
		else                 s += GRAY + name + RST;
		s += GRAY + fmt(" %-14s", hints[p]) + RST;
		if(p < 2) s += GRAY + "▶  " + RST;
	}
	return s;
}

//the bus column: a path from row "from" to row "to", carrying payload. Going right = into the CPU.
Lines busColumn(int from, int to, const std::string& payload, bool intoCpu){
	Lines col(BODY_H + 2, std::string(BUS_W, ' '));
	col[0] = GRAY + pad("     BUS", BUS_W) + RST;
	if(from < 0) return col;
	int srcLine = from + 1, dstLine = to + 1;				//+1 for the top border
	std::string tag = hi(payload);
	int fill = BUS_W - 3 - vlen(payload);
	if(intoCpu){
		if(srcLine == dstLine){
			col[srcLine] = YEL + "─" + RST + tag + YEL + rep("─", fill + 1) + "▶" + RST;
			return col;
		}
		col[srcLine] = YEL + "─" + RST + tag + YEL + rep("─", fill) + "┘" + RST + " ";
		for(int r = dstLine + 1; r < srcLine; ++r) col[r] = std::string(BUS_W - 2, ' ') + YEL + "│" + RST + " ";
		col[dstLine] = std::string(BUS_W - 2, ' ') + YEL + "┌▶" + RST;
	}else{
		//out of the CPU at row "from", down to memory at row "to"
		col[srcLine] = std::string(BUS_W - 2, ' ') + YEL + "┌─" + RST;
		for(int r = srcLine + 1; r < dstLine; ++r) col[r] = std::string(BUS_W - 2, ' ') + YEL + "│" + RST + " ";
		col[dstLine] = YEL + "◀" + RST + tag + YEL + rep("─", fill) + "┘" + RST + " ";
	}
	return col;
}

int indexOf(const std::vector<Instr>& prog, Byte opcode){
	for(size_t i = 0; i < prog.size(); ++i) if(prog[i].bytes[0] == opcode) return (int)i;
	return -1;
}

//ticks of each phase, one part lit per tick:
//  FETCH    0 PC   1 ROM bytes   2 bus   3 Instruction Register
//  DECODE   0 IR   1 decoder: which instruction   2 decoder: what it means
//  EXECUTE  0 decoder, then  LDI: 1 register  2 PC
//                            ADD: 1 registers out   2 ALU   3 result into R0, PC
//                            ST:  1 R0 out on the bus   2 RAM written, PC
Lines diagram(const View& v, int tick){
	bool fetch = v.phase == FETCH, decode = v.phase == DECODE, exec = v.phase == EXECUTE;
	const std::vector<Byte>& b = v.fetched;
	Byte op = b[0];
	bool done = exec && tick >= lastTick(v);		//this instruction has finished

	//---- MEMORY
	Lines mem;
	mem.push_back(GRAY + "PROGRAM (ROM)" + RST);
	for(size_t i = 0; i < v.prog.size(); ++i){
		bool current = (int)i == v.instr;
		bool past    = (int)i < v.instr || (current && done);
		std::string bytes = hexBytes(v.prog[i].bytes);
		if(current && fetch && tick >= 1) bytes = hi(bytes);
		std::string mark = past ? GRN + "✓" + RST : current ? YEL + "▶" + RST : " ";
		mem.push_back(mark + fmt(" %04X  ", v.prog[i].addr) + bytes);
	}
	mem.push_back("");
	mem.push_back(GRAY + "DATA (RAM)" + RST);
	bool storing = exec && op == OP_ST;
	bool stored  = (storing && tick >= 2) || v.instr > indexOf(v.prog, OP_ST);
	std::string cell = fmt("%02X", storing && !stored ? v.ramBefore : v.ramAfter);
	mem.push_back(fmt("  %04X  ", ANSWER) + (storing && stored ? hi(cell) : cell) +
	              (stored ? GRN + fmt("  = %d", v.ramAfter) + RST : GRAY + "  answer" + RST));

	//---- CPU
	bool pcLit = (fetch && tick == 0) || (done && op != OP_HLT);
	Word pc    = done ? v.pcAfter : v.pcBefore;
	bool irFull = !(fetch && tick < 3);
	bool irLit  = (fetch && tick >= 3) || (decode && tick == 0);
	std::string ir = irFull ? hexBytes(b) : GRAY + "(empty)" + RST;
	bool decLit = (decode && tick >= 1) || (exec && tick == 0);
	std::string dec1 = GRAY + "-" + RST, dec2;
	if(decode ? tick >= 1 : exec){
		dec1 = fmt("%02X = %s (%d bytes)", op, v.info->mnemonic, v.info->length);
		if(decLit) dec1 = YEL + BOLD + dec1 + RST;
	}
	if(decode ? tick >= 2 : exec) dec2 = (decLit ? YEL : GRAY) + meaning(b) + RST;

	int target  = op == OP_LDI ? b[1] : op == OP_ADD ? b[1] >> 4 : -1;	//register this instruction writes
	int writeAt = op == OP_ADD ? 3 : 1;
	std::string regs = "Registers       ";
	for(int r = 0; r < 2; ++r){
		bool written = exec && r == target && tick >= writeAt;
		bool lit = written ||
		           (exec && op == OP_ADD && (tick == 1 || tick == 2)) ||	//both go into the ALU
		           (exec && op == OP_ST && r == b[1] && tick == 1);		//R0 goes out to memory
		std::string text = fmt("[%3d]", written ? v.regAfter[r] : v.regBefore[r]);
		regs += fmt(r ? "  R%d " : "R%d ", r) + (lit ? hi(text) : text);
	}

	bool adding = exec && op == OP_ADD;
	std::string wires, alu = GRAY + "idle" + RST;
	if(adding && tick >= 1) wires = std::string(21, ' ') + YEL + "└────┬────┘" + RST;
	if(adding && tick >= 2){
		alu = hi(fmt("%d + %d = %d", v.regBefore[0], v.regBefore[1], v.regAfter[0]));
		if(tick >= 3) alu += YEL + " → R0" + RST;
	}else if(v.instr > indexOf(v.prog, OP_ADD)){
		alu = GRAY + fmt("%d + %d = %d", v.a, v.b, (Byte)(v.a + v.b)) + RST;
	}

	Lines cpu;
	cpu.push_back(label("Program Counter ", pcLit) + (pcLit ? hi(fmt("%04X", pc)) : fmt("%04X", pc)) +
	              GRAY + "  next address" + RST);
	cpu.push_back(label("Instruction Reg ", irLit) + (irLit ? hi(ir) : ir));
	cpu.push_back(label("Decoder         ", decLit) + dec1);
	cpu.push_back("                " + dec2);
	cpu.push_back("");
	cpu.push_back(regs);
	cpu.push_back(wires);
	cpu.push_back(label("ALU             ", adding && tick >= 2) + alu);
	cpu.push_back("");
	if(exec && op == OP_HLT) cpu.push_back(GRN + BOLD + "■ STOPPED" + RST);

	//---- BUS: drawn only while bytes are on it
	Lines bus;
	if(fetch && tick >= 2)
		bus = busColumn(romRow(v.instr), ROW_IR, "[" + hexBytes(b) + "]", true);
	else if(storing && tick >= 1)
		bus = busColumn(ROW_REGS, ROW_RAM, fmt("[%02X]", v.regBefore[b[1]]), false);
	else
		bus = busColumn(-1, -1, "", true);

	Lines left  = box("MEMORY", MEM_INNER, mem, BODY_H);
	Lines right = box("CPU", CPU_INNER, cpu, BODY_H);
	Lines out;
	for(size_t r = 0; r < left.size(); ++r) out.push_back(left[r] + bus[r] + right[r]);
	return out;
}

//------------------------------------------------------------------------------------------------
//WHAT TO SAY: one short paragraph per phase, with the real numbers

std::string story(const View& v){
	const std::vector<Byte>& b = v.fetched;
	std::string bytes = hexBytes(b);
	if(v.phase == FETCH){
		std::string s = fmt("The Program Counter holds %04X, the address of the next instruction. "
		                    "The CPU sends this address over the bus to memory, and memory sends back "
		                    "the bytes stored there: %s. They land in the Instruction Register.",
		                    v.pcBefore, bytes.c_str());
		if(v.instr == 0) s += " Memory only stores numbers. It has no idea what they mean.";
		return s;
	}
	if(v.phase == DECODE){
		switch(b[0]){
			case OP_LDI:
				return fmt("The Decoder looks up the first byte, %02X, in its instruction table: %02X means "
				           "LDI, \"load a number\", 3 bytes long. The next byte, %02X, picks register R%d. "
				           "The last byte is the number itself: %d%s.", b[0], b[0], b[1], b[1], b[2],
				           b[2] > 9 ? fmt(" (written %02X in hexadecimal)", b[2]).c_str() : "");
			case OP_ADD:
				return fmt("%02X means ADD, 2 bytes long. The second byte, %02X, is read as two halves: "
				           "%d means R%d (the first number, and where the answer will go) and %d means R%d "
				           "(the second number).", b[0], b[1], b[1] >> 4, b[1] >> 4, b[1] & 15, b[1] & 15);
			case OP_ST:
				return fmt("%02X means ST, \"store\": copy a register into memory. 4 bytes long. %02X picks "
				           "R%d. The last two bytes, %02X %02X, make the address %04X (the small half comes first).",
				           b[0], b[1], b[1], b[2], b[3], makeWord(b[2], b[3]));
			case OP_HLT:
				return fmt("%02X means HLT, \"halt\": the program is finished. It is only 1 byte long.", b[0]);
		}
	}
	switch(b[0]){
		case OP_LDI:
			return fmt("The number %d goes into register R%d. Registers are a few tiny storage boxes "
			           "inside the CPU. The CPU can only calculate with values that are in registers. "
			           "The Program Counter moves on to %04X, the next instruction.", b[2], b[1], v.pcAfter);
		case OP_ADD:{
			int real = v.regBefore[0] + v.regBefore[1];
			std::string s = fmt("R0 (%d) and R1 (%d) are sent to the ALU (Arithmetic Logic Unit), the "
			                    "CPU's calculator. It adds them: %d + %d = %d. The result is written back "
			                    "into R0, replacing the %d.", v.regBefore[0], v.regBefore[1],
			                    v.regBefore[0], v.regBefore[1], v.regAfter[0], v.regBefore[0]);
			if(v.carry)
				s += fmt(" (The true answer %d does not fit in one byte, max 255, so only %d is kept "
				         "and the Carry flag turns on.)", real, v.regAfter[0]);
			return s;
		}
		case OP_ST:
			return fmt("R0 holds the answer, %d. It travels over the bus again, this time out of the CPU, "
			           "and is written into RAM at address %04X. Unlike ROM, RAM can be written to. "
			           "The answer is now saved in memory.", v.regBefore[0], ANSWER);
		case OP_HLT:
			return "The CPU stops. The program is finished. Press Enter to see the result.";
	}
	return "";
}

//------------------------------------------------------------------------------------------------
//THE THREE KINDS OF SCREEN

std::string titleBar(const std::string& right){
	return ON_CYN + BOLD + pad(" PIXEL-8 CPU ▸ one addition, step by step", SCREEN_W - vlen(right)) + right + RST;
}

std::string keysLine(){
	return GRAY + " Enter = next   ← / B = back   R = restart   N = new numbers   Q = quit" + RST;
}

Lines introScreen(Byte a, Byte b){
	std::vector<Instr> prog = splitProgram(makeProgram(a, b));
	Lines s;
	s.push_back(titleBar(""));
	s.push_back("");
	s.push_back(BOLD + fmt("  Goal: watch the CPU work out  %d + %d", a, b) + RST);
	s.push_back("");
	s.push_back("  A CPU cannot read \"" + fmt("%d + %d", a, b) + "\". A programmer writes small instructions,");
	s.push_back("  and an assembler turns them into numbers (bytes) stored in memory:");
	s.push_back("");
	s.push_back(GRAY + "     #  instruction       meaning                         bytes in memory" + RST);
	for(size_t i = 0; i < prog.size(); ++i)
		s.push_back(fmt("     %d  ", (int)i + 1) + CYAN + pad(asmText(prog[i].bytes), 18) + RST +
		            pad(meaning(prog[i].bytes), 32) + YEL + hexBytes(prog[i].bytes) + RST);
	s.push_back("");
	s.push_back("  The CPU handles every instruction in the same three phases:");
	s.push_back("     " + ON_YEL + " FETCH   " + RST + "  get the instruction's bytes from memory");
	s.push_back("     " + ON_YEL + " DECODE  " + RST + "  work out what those bytes mean");
	s.push_back("     " + ON_YEL + " EXECUTE " + RST + "  do it");
	s.push_back("");
	s.push_back(GRAY + "  Addresses and bytes are in hexadecimal, as usual for memory." + RST);
	s.push_back(GRAY + "  Numbers inside registers are shown as normal numbers." + RST);
	s.push_back("");
	if(terminalWidth() < SCREEN_W + 1)
		s.push_back(YEL + fmt("  Make this window at least %d columns wide (it is %d now).", SCREEN_W + 1, terminalWidth()) + RST);
	s.push_back(BOLD + "  Press Enter to start." + RST);
	s.push_back("");
	s.push_back(keysLine());
	return s;
}

Lines phaseScreen(const View& v, int step, int total, int tick){
	Lines s;
	s.push_back(titleBar(fmt("step %d of %d ", step, total)));
	s.push_back(BOLD + fmt(" Instruction %d of %d: ", v.instr + 1, (int)v.prog.size()) + RST +
	            (v.phase == FETCH ? GRAY + "(not known yet)" + RST
	                              : CYAN + asmText(v.fetched) + RST + GRAY + "   " + meaning(v.fetched) + RST));
	s.push_back(" " + phaseStrip(v));
	s.push_back("");
	Lines d = diagram(v, tick);
	s.insert(s.end(), d.begin(), d.end());

	const char* names[] = { "FETCH", "DECODE", "EXECUTE" };
	Lines words = wrap(story(v), SCREEN_W - 4);
	Lines body;
	for(size_t i = 0; i < words.size(); ++i) body.push_back(words[i]);
	Lines say = box(std::string("WHAT IS HAPPENING: ") + names[v.phase], SCREEN_W - 4, body, 5);
	s.insert(s.end(), say.begin(), say.end());
	s.push_back(keysLine());
	return s;
}

Lines resultScreen(Byte a, Byte b){
	Machine m(makeProgram(a, b));
	int instructions = 0;
	while(m.cpu.getState() == CpuState::Running){ m.cpu.step(); ++instructions; }
	Byte answer   = m.bus.peek(ANSWER);
	Byte expected = (Byte)(a + b);		//plain C++, to check the emulator

	Lines s;
	s.push_back(titleBar("finished "));
	s.push_back("");
	s.push_back(BOLD + "  RESULT" + RST);
	s.push_back("");
	s.push_back("      " + ON_GRN + BOLD + fmt("  %d + %d = %d  ", a, b, answer) + RST);
	s.push_back("");
	s.push_back(fmt("  Read back from RAM address %04X: %d   ", ANSWER, answer) +
	            (answer == expected ? GRN + "✓ matches the expected answer" : RED + "✗ expected " + fmt("%d", expected)) + RST);
	if(a + b > 255)
		s.push_back(YEL + fmt("  (%d + %d = %d does not fit in 8 bits, so it wrapped around. Carry flag: %s)",
		                      a, b, a + b, m.cpu.getFlag(CPU::FLAG_C) ? "on" : "off") + RST);
	s.push_back("");
	s.push_back(fmt("  The CPU ran %d instructions. Every one went through the same three phases:", instructions));
	s.push_back("     " + ON_GRN + " FETCH   " + RST + "  got the bytes from memory, over the bus");
	s.push_back("     " + ON_GRN + " DECODE  " + RST + "  the decoder worked out what they mean");
	s.push_back("     " + ON_GRN + " EXECUTE " + RST + "  registers, ALU and memory did the work");
	s.push_back("");
	s.push_back(fmt("  Clock cycles used: %lu.", m.cpu.getCycles()));
	s.push_back("");
	s.push_back("  The journey of the numbers:");
	s.push_back(CYAN + "     ROM → bus → Instruction Reg → Decoder" + RST);
	s.push_back(CYAN + "         → Registers → ALU → Registers → bus → RAM" + RST);
	s.push_back("");
	s.push_back("  A real computer does exactly this, just billions of times per second.");
	s.push_back("");
	s.push_back(keysLine());
	return s;
}

//------------------------------------------------------------------------------------------------

//read a number 0-255 on a normal (non-raw) line. Returns false if nothing valid was typed.
bool askNumber(const char* prompt, Byte& out){
	printf("\x1b[?25h  %s (0-255): ", prompt);
	fflush(stdout);
	char text[64];
	if(!fgets(text, sizeof(text), stdin)) return false;
	char* end = nullptr;
	long n = strtol(text, &end, 10);
	printf("\x1b[?25l");
	if(end == text || n < 0 || n > 255) return false;
	out = (Byte)n;
	return true;
}

void askNumbers(Byte& a, Byte& b){
	show(Lines{ titleBar(""), "", BOLD + "  Choose two numbers to add" + RST,
	            GRAY + "  (press Enter without typing to keep the current one)" + RST, "" });
	printf("\x1b[6;1H");
	askNumber(fmt("First number  [%d]", a).c_str(), a);
	askNumber(fmt("Second number [%d]", b).c_str(), b);
}

bool parseArg(const char* text, Byte& out){
	char* end = nullptr;
	long n = strtol(text, &end, 10);
	if(end == text || *end != '\0' || n < 0 || n > 255) return false;
	out = (Byte)n;
	return true;
}

int main(int argc, char** argv){
	Byte a = 5, b = 3;
	if(argc >= 3 && !(parseArg(argv[1], a) && parseArg(argv[2], b))){
		printf("usage: add [A B]   two numbers from 0 to 255\n");
		return 1;
	}
	if(!setupTerminal()){
		printf("This console cannot show colours. Please use Windows Terminal or PowerShell.\n");
		return 1;
	}

	int total = 3 * (int)splitProgram(makeProgram(a, b)).size();
	int step = 0;						//0 = intro, 1..total = phases, total+1 = result
	bool forward = false;				//animate only when moving forward

	for(;;){
		if(step == 0)          show(introScreen(a, b));
		else if(step > total)  show(resultScreen(a, b));
		else{
			View v = buildView(a, b, step);
			int last = lastTick(v);
			for(int t = forward ? 0 : last; t <= last; ++t){
				show(phaseScreen(v, step, total, t));
				if(t == last) break;
				sleepMs(650);
				if(keyWaiting()){ getKey(); t = last - 1; }		//a key skips to the end of the animation
			}
		}

		int key = getKey();
		forward = false;
		if(key == K_EOF || key == 'q' || key == 'Q' || key == K_ESC) break;
		if(key == K_ENTER || key == ' ' || key == K_RIGHT){
			if(step <= total){ ++step; forward = true; }
		}
		else if(key == K_BACK || key == K_LEFT || key == 'b' || key == 'B'){
			if(step > 0) --step;
		}
		else if(key == 'r' || key == 'R') step = 0;
		else if(key == 'n' || key == 'N'){
			askNumbers(a, b);
			step = 0;
		}
	}
	return 0;
}
