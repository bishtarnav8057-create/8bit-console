//LIVE DASHBOARD DEMO OF THE PIXEL-8 CPU AND MEMORY
//run: mingw32-make live        (make the terminal at least 100 columns wide)
//one screen that redraws on every key press: the program, registers, flags, stack or screen,
//and one plain sentence about what the last instruction did. The keys are listed on every screen.
//the old scrolling demo is still there: mingw32-make demo

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <string>
#include <vector>
#include <functional>

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
//TERMINAL: colours, keys, drawing one whole screen at a time

const int SCREEN_W = 100;

const std::string RST    = "\x1b[0m";
const std::string BOLD   = "\x1b[1m";
const std::string GRAY   = "\x1b[90m";
const std::string CYAN   = "\x1b[96m";
const std::string YEL    = "\x1b[93m";
const std::string GRN    = "\x1b[92m";
const std::string RED    = "\x1b[91m";
const std::string MAG    = "\x1b[95m";
const std::string WHITE  = "\x1b[97m";
const std::string ON_GRN = "\x1b[30;102m";		//black text on green: a lit lamp
const std::string ON_YEL = "\x1b[30;103m";
const std::string ON_RED = "\x1b[97;41m";
const std::string ON_CYN = "\x1b[30;106m";

enum Key { K_NONE = 0, K_ENTER = 13, K_BACK = 8, K_ESC = 27,
           K_UP = 1000, K_DOWN, K_LEFT, K_RIGHT, K_HOME, K_EOF };

bool keyboard = false;		//true when keys come from a real console, false when piped in (tests)
bool quitAll = false;		//Ctrl+C or end of piped input: leave every screen

void restoreTerminal(){
	printf("\x1b[0m\x1b[?25h\x1b[?1049l");		//colours off, cursor back, normal screen back
	fflush(stdout);
}

#ifdef _WIN32
BOOL WINAPI onCtrl(DWORD){
	restoreTerminal();
	return FALSE;
}
#endif

//colours need "virtual terminal" mode on Windows. false if this console cannot do it.
bool setupTerminal(){
#ifdef _WIN32
	DWORD mode = 0;
	keyboard = GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode) != 0;
	SetConsoleOutputCP(65001);					//UTF-8, for the box lines
	HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
	if(GetConsoleMode(out, &mode)){
		if(!SetConsoleMode(out, mode | 0x0004)) return false;	//ENABLE_VIRTUAL_TERMINAL_PROCESSING (old MinGW headers miss the name)
	}
	SetConsoleCtrlHandler(onCtrl, TRUE);
#endif
	printf("\x1b[?1049h\x1b[?25l");			//own screen, hide the cursor
	atexit(restoreTerminal);
	return true;
}

int terminalWidth(){
#ifdef _WIN32
	CONSOLE_SCREEN_BUFFER_INFO info;
	if(GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info))
		return info.srWindow.Right - info.srWindow.Left + 1;
#endif
	return SCREEN_W;
}

int getKey(){
	int c;
#ifdef _WIN32
	if(keyboard){
		c = _getch();
		if(c == 0 || c == 224){					//arrow keys come as two codes
			switch(_getch()){
				case 72: return K_UP;
				case 80: return K_DOWN;
				case 75: return K_LEFT;
				case 77: return K_RIGHT;
				case 71: return K_HOME;
			}
			return K_NONE;
		}
		if(c == 3){ quitAll = true; return K_EOF; }		//Ctrl+C
		return c;
	}
#endif
	c = getchar();
	while(c == '\r') c = getchar();
	if(c == EOF){ quitAll = true; return K_EOF; }
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
	if(!keyboard || ms <= 0) return;			//piped input: no animation
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

//characters on screen: colour codes take no room, a UTF-8 character is one column
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

void add(Lines& to, const Lines& more){
	to.insert(to.end(), more.begin(), more.end());
}

//draw the whole screen in one write, from the top-left corner, so nothing flickers
void show(const Lines& lines){
	std::string out = "\x1b[H";
	for(size_t i = 0; i < lines.size(); ++i) out += lines[i] + RST + "\x1b[K\n";
	out += "\x1b[J";
	fwrite(out.data(), 1, out.size(), stdout);
	fflush(stdout);
}

//a panel with a title in the top border. padded = one space inside each side
Lines box(const std::string& title, int inner, const Lines& body, int height, bool padded = true){
	int width = inner + (padded ? 4 : 2);
	Lines out;
	out.push_back(GRAY + "┌─ " + RST + BOLD + CYAN + title + RST + GRAY + " " + rep("─", width - 5 - vlen(title)) + "┐" + RST);
	for(int i = 0; i < height; ++i){
		std::string line = i < (int)body.size() ? body[i] : "";
		if(padded) out.push_back(GRAY + "│" + RST + " " + pad(line, inner) + RST + " " + GRAY + "│" + RST);
		else       out.push_back(GRAY + "│" + RST + pad(line, inner) + RST + GRAY + "│" + RST);
	}
	out.push_back(GRAY + "└" + rep("─", width - 2) + "┘" + RST);
	return out;
}

//panels side by side, one space apart
Lines hjoin(const std::vector<Lines>& panels){
	size_t rows = 0;
	std::vector<int> widths;
	for(size_t i = 0; i < panels.size(); ++i){
		widths.push_back(panels[i].empty() ? 0 : vlen(panels[i][0]));
		if(panels[i].size() > rows) rows = panels[i].size();
	}
	Lines out;
	for(size_t r = 0; r < rows; ++r){
		std::string line;
		for(size_t i = 0; i < panels.size(); ++i){
			if(i) line += " ";
			line += r < panels[i].size() ? panels[i][r] : std::string(widths[i], ' ');
		}
		out.push_back(line);
	}
	return out;
}

//split plain text (no colour codes) into lines of at most width characters, at spaces
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

std::string bar(const std::string& colour, const std::string& text){
	return colour + pad(text, SCREEN_W) + RST;
}

std::string header(const std::string& left, const std::string& right){
	return ON_CYN + BOLD + pad(" PIXEL-8 ▸ " + left, SCREEN_W - vlen(right)) + right + RST;
}

std::string footer(const std::string& keys){
	return GRAY + " " + keys + RST;
}

std::string bin(Byte v){
	std::string s;
	for(int i = 7; i >= 0; --i){
		s += getBit(v, i) ? '1' : '0';
		if(i == 4) s += ' ';
	}
	return s;
}

std::string lamp(const char* name, bool on){
	return on ? ON_GRN + " " + name + " " + RST : GRAY + " " + name + " " + RST;
}

std::string lamps(Byte flags){
	return lamp("Z zero", getBit(flags, CPU::FLAG_Z)) + " " + lamp("C carry", getBit(flags, CPU::FLAG_C)) + " " +
	       lamp("N negative", getBit(flags, CPU::FLAG_N)) + " " + lamp("V overflow", getBit(flags, CPU::FLAG_V));
}

//------------------------------------------------------------------------------------------------
//THE MACHINE: the memory chips wired like MemoryMap.h. Like demo.cpp there are no I/O ports here,
//so 0xF000 is empty (scene 5 uses that). Scene 7 uses the real Console with its ports.

struct WriteRec{
	Word address;
	Byte oldValue;
	Byte newValue;
	int  prevOwner;		//which step wrote this address before, so step back can put it back
};

struct Board{
	Rom rom;
	Ram ram;
	Ram vram;
	Ram stack;
	Bus bus;
	CPU cpu;
	Disassembler dis;
	std::vector<WriteRec>* log;		//where the write listener records, while a step runs

	Board() : rom(0x8000), ram(0x6000, "RAM"), vram(0x1000, "VRAM"), stack(0x0F00, "STACK"),
	          bus(), cpu(bus), dis(bus), log(nullptr){
		bus.attach(MemoryMap::ROM_START,   MemoryMap::ROM_END,   &rom);
		bus.attach(MemoryMap::RAM_START,   MemoryMap::RAM_END,   &ram);
		bus.attach(MemoryMap::VRAM_START,  MemoryMap::VRAM_END,  &vram);
		bus.attach(MemoryMap::STACK_START, MemoryMap::STACK_END, &stack);
		//the same hook Piyush's State Manager uses for undo
		bus.setWriteListener([this](Word address, Byte oldValue, Byte newValue){
			if(log) log->push_back({address, oldValue, newValue, -1});
		});
	}

	void load(const std::vector<Byte>& program){
		rom.load(program.data(), (unsigned int)program.size());
		cpu.reset();
	}
};

const char* chipName(Word address){
	if(address <= MemoryMap::ROM_END)  return "ROM";
	if(address <= MemoryMap::RAM_END)  return "RAM";
	if(address <= MemoryMap::VRAM_END) return "VRAM";
	if(address <= MemoryMap::IO_END)   return "I/O";
	return "stack";
}

std::string pixelNote(Word address){
	int offset = address - MemoryMap::VRAM_START;
	if(address < MemoryMap::VRAM_START || offset >= MemoryMap::SCREEN_WIDTH * MemoryMap::SCREEN_HEIGHT) return "";
	return fmt(" = pixel (%d, %d)", offset % MemoryMap::SCREEN_WIDTH, offset / MemoryMap::SCREEN_WIDTH);
}

const char* formatName(Format format){
	switch(format){
		case Format::Implied: return "no operands";
		case Format::Reg:     return "1 register";
		case Format::RegReg:  return "2 registers";
		case Format::RegImm:  return "reg + number";
		case Format::Abs:     return "address";
		case Format::RegAbs:  return "reg + address";
		case Format::RegInd:  return "reg + pointer P";
	}
	return "";
}

//everything the CPU holds, so step back can put it all back
struct Snap{
	Byte reg[CPU::NUM_REGS];
	Word pc;
	Word sp;
	Byte flags;
	unsigned long cycles;
};

Snap snap(const CPU& cpu){
	Snap s;
	for(int i = 0; i < CPU::NUM_REGS; ++i) s.reg[i] = cpu.getReg(i);
	s.pc = cpu.getPC();
	s.sp = cpu.getSP();
	s.flags = cpu.getFlags();
	s.cycles = cpu.getCycles();
	return s;
}

//------------------------------------------------------------------------------------------------
//THE STEPPER: runs a program one instruction at a time on the dashboard

struct Label{
	Word address;
	const char* name;
};

struct Scene{
	std::string title;
	std::string goal;
	std::vector<Byte> program;
	std::vector<Label> labels;
	bool screen;			//side panel shows VRAM as a picture instead of the stack
	Word watch;				//a RAM address to show (0 = none)
	const char* watchName;
	int delayMs;			//R: pause between steps
	unsigned long slowSteps;	//R: after this many steps, run flat out
	std::function<std::string(Board&, unsigned long steps)> summary;

	Scene() : screen(false), watch(0), watchName(""), delayMs(150), slowSteps(100000) {}
};

struct StepRec{
	Snap before;
	Byte op;
	int  reg;							//the register the instruction names first
	std::vector<WriteRec> writes;
	std::string strip;					//FETCH ▸ DECODE ▸ EXECUTE
	std::string what;					//the plain sentence
	std::string extra;					//flags that changed, or why a fault happened
};

class Stepper{
	public:
		explicit Stepper(const Scene& scene) : sc(scene), owner(0x10000, -1){
			board.load(sc.program);
			Word address = 0;
			while(address < sc.program.size()){
				Byte length = 0;
				board.dis.line(address, length);
				addrs.push_back(address);
				address = (Word)(address + length);
			}
		}

		void run(){
			draw();
			while(!quitAll){
				int k = getKey();
				if(k == K_ENTER || k == ' ' || k == K_RIGHT || k == 'n') step();
				else if(k == K_BACK || k == K_LEFT || k == 'b') back();
				else if(k == 'r' || k == 'R') runToEnd();
				else if(k == 's' || k == 'S' || k == K_HOME) { while(!hist.empty()) back(); }
				else if(k == K_ESC || k == 'q' || k == 'Q' || k == K_EOF) return;
				else continue;
				draw();
			}
		}

	private:
		const Scene& sc;
		Board board;
		std::vector<Word> addrs;			//where each instruction of the program starts
		std::vector<StepRec> hist;
		std::vector<int> owner;				//for each address, the step that last wrote it (stack labels)

		bool running(){ return board.cpu.getState() == CpuState::Running; }

		std::string labelAt(Word address){
			for(size_t i = 0; i < sc.labels.size(); ++i) if(sc.labels[i].address == address) return sc.labels[i].name;
			return "";
		}

		std::string where(Word address){
			std::string name = labelAt(address);
			return name.empty() ? fmt("0x%04X", address) : name;
		}

		//the Disassembler's text, with jump targets shown as their label names
		std::string instrText(Word address){
			Byte length = 0;
			std::string text = board.dis.line(address, length).substr(8);
			for(size_t i = 0; i < sc.labels.size(); ++i){
				std::string target = fmt("0x%04X", sc.labels[i].address);
				size_t at = text.find(target);
				if(at != std::string::npos) text.replace(at, target.size(), sc.labels[i].name);
			}
			return text;
		}

		bool step(){
			if(!running()) return false;
			StepRec r;
			r.before = snap(board.cpu);
			Byte bytes[4];
			for(int i = 0; i < 4; ++i) bytes[i] = board.bus.peek((Word)(r.before.pc + i));
			r.op = bytes[0];
			r.reg = (OpcodeTable::lookup(r.op).format == Format::RegReg) ? (bytes[1] >> 4) : bytes[1];

			board.log = &r.writes;
			board.cpu.step();
			board.log = nullptr;

			for(size_t i = 0; i < r.writes.size(); ++i){
				r.writes[i].prevOwner = owner[r.writes[i].address];
				owner[r.writes[i].address] = (int)hist.size();
			}
			explain(r, bytes);
			hist.push_back(r);
			return true;
		}

		//undo: memory first (newest write first), then the CPU, exactly how the State Manager will do it
		void back(){
			if(hist.empty()) return;
			const StepRec& r = hist.back();
			for(size_t i = r.writes.size(); i-- > 0; ){
				board.bus.poke(r.writes[i].address, r.writes[i].oldValue);
				owner[r.writes[i].address] = r.writes[i].prevOwner;
			}
			for(int i = 0; i < CPU::NUM_REGS; ++i) board.cpu.setReg(i, r.before.reg[i]);
			board.cpu.setPC(r.before.pc);
			board.cpu.setSP(r.before.sp);
			board.cpu.setFlags(r.before.flags);
			board.cpu.setCycles(r.before.cycles);
			board.cpu.setState(CpuState::Running);		//it was running, or it could not have stepped
			board.bus.clearFault();
			hist.pop_back();
		}

		void runToEnd(){
			unsigned long drawn = 0;
			while(running() && !quitAll){
				step();
				bool slow = hist.size() <= sc.slowSteps;
				if(keyboard && (slow || hist.size() - drawn >= 64)){
					draw();
					drawn = hist.size();
					if(slow) sleepMs(sc.delayMs);
				}
				if(keyWaiting()){ getKey(); break; }		//any key pauses
			}
		}

		void explain(StepRec& r, const Byte* bytes){
			const OpcodeInfo& info = OpcodeTable::lookup(r.op);
			const Snap& was = r.before;
			Snap now = snap(board.cpu);
			bool faulted = board.cpu.getState() == CpuState::Faulted;

			//FETCH ▸ DECODE ▸ EXECUTE
			int length = info.valid ? info.length : 1;
			std::string fetched;
			for(int i = 0; i < length; ++i) fetched += fmt(i ? " %02X" : "%02X", bytes[i]);
			r.strip = GRAY + "FETCH " + RST + WHITE + fetched + RST + GRAY + fmt(" from 0x%04X", was.pc) + "  ▸  DECODE " + RST;
			if(info.valid) r.strip += WHITE + fmt("0x%02X = %s", r.op, info.mnemonic) + RST + GRAY + fmt(", %s, %d byte%s", formatName(info.format), length, length == 1 ? "" : "s") + RST;
			else           r.strip += RED + fmt("0x%02X is not an instruction!", r.op) + RST;
			r.strip += GRAY + "  ▸  EXECUTE " + RST;
			r.strip += faulted ? RED + "FAULT" + RST : WHITE + fmt("+%d cycles", info.cycles) + RST;

			if(faulted){
				const Fault& f = board.cpu.getFault();
				r.what  = RED + fmt("✗ %s: %s", faultName(f.type), f.message.c_str()) + RST;
				r.extra = "The CPU stopped itself. Nothing crashed, and PC still points at the bad instruction.";
				return;
			}

			int d = r.reg & 7, s = bytes[1] & 0x0F & 7;
			Byte imm = bytes[2];
			Word target = makeWord(bytes[1], bytes[2]);		//JMP / CALL
			Word address = makeWord(bytes[2], bytes[3]);		//LD / ST
			Word p = makeWord(was.reg[7], was.reg[6]);
			int x = was.reg[d], y = was.reg[s], res = now.reg[d];
			int carry = getBit(was.flags, CPU::FLAG_C) ? 1 : 0;
			std::string note;

			switch(r.op){
				case OP_NOP:  r.what = "Do nothing and move on."; break;
				case OP_HLT:  r.what = "HLT: stop the CPU. The program is finished."; break;
				case OP_MOV:  r.what = fmt("Copy R%d into R%d  →  R%d = %d", s, d, d, res); break;
				case OP_LDI:  r.what = fmt("Put the number %d (0x%02X) into R%d", imm, imm, d); break;
				case OP_LD:   r.what = fmt("Read memory [0x%04X] (%s) into R%d  →  %d", address, chipName(address), d, res); break;
				case OP_ST:   r.what = fmt("Write R%d = %d into memory [0x%04X], the %s chip%s", d, x, address, chipName(address), pixelNote(address).c_str()); break;
				case OP_LDP:  r.what = fmt("Read memory at P = R6:R7 = 0x%04X into R%d  →  %d", p, d, res); break;
				case OP_STP:  r.what = fmt("Write R%d = %d to the address in P = R6:R7 = 0x%04X%s", d, x, p, pixelNote(p).c_str()); break;
				case OP_INP:
				case OP_DEP:{
					Word np = makeWord(now.reg[7], now.reg[6]);
					r.what = fmt("P = P %s 1  →  0x%04X%s", r.op == OP_INP ? "+" : "−", np, pixelNote(np).c_str());
					break;
				}
				case OP_ADD:
					r.what = fmt("R%d = R%d + R%d  →  %d + %d = %d", d, d, s, x, y, res);
					if(x + y > 255) note = fmt("%d does not fit in 8 bits, so it wrapped round to %d. ", x + y, res);
					break;
				case OP_ADC:
					r.what = fmt("R%d = R%d + R%d + carry  →  %d + %d + %d = %d", d, d, s, x, y, carry, res);
					if(d == 6) note = "Adding the carry into R6 moves P on to the next 256 bytes. ";
					break;
				case OP_SUB:  r.what = fmt("R%d = R%d − R%d  →  %d − %d = %d", d, d, s, x, y, res); break;
				case OP_SBC:  r.what = fmt("R%d = R%d − R%d − borrow  →  %d − %d − %d = %d", d, d, s, x, y, carry, res); break;
				case OP_AND:
				case OP_OR:
				case OP_XOR:
					r.what = fmt("R%d = R%d %s R%d  →  %s %s %s = %s", d, d, info.mnemonic, s,
					             bin(x).c_str(), info.mnemonic, bin(y).c_str(), bin(res).c_str());
					break;
				case OP_CMP:  r.what = fmt("Compare R%d with R%d: work out %d − %d, keep only the flags (R%d stays %d)", d, s, x, y, d, res); break;
				case OP_ADDI:
					r.what = fmt("R%d = R%d + %d  →  %d + %d = %d", d, d, imm, x, imm, res);
					if(x + imm > 255) note = fmt("%d does not fit in 8 bits, so it wrapped round to %d. ", x + imm, res);
					break;
				case OP_SUBI: r.what = fmt("R%d = R%d − %d  →  %d − %d = %d", d, d, imm, x, imm, res); break;
				case OP_CMPI: r.what = fmt("Compare R%d with %d: work out %d − %d, keep only the flags", d, imm, x, imm); break;
				case OP_INC:  r.what = fmt("R%d = R%d + 1  →  %d", d, d, res); break;
				case OP_DEC:  r.what = fmt("R%d = R%d − 1  →  %d", d, d, res); break;
				case OP_NOT:  r.what = fmt("Flip every bit of R%d:  %s  →  %s", d, bin(x).c_str(), bin(res).c_str()); break;
				case OP_SHL:  r.what = fmt("Shift R%d left (×2):  %s  →  %s", d, bin(x).c_str(), bin(res).c_str()); break;
				case OP_SHR:  r.what = fmt("Shift R%d right (÷2):  %s  →  %s", d, bin(x).c_str(), bin(res).c_str()); break;
				case OP_JMP:  r.what = fmt("Jump to %s", where(target).c_str()); break;
				case OP_JZ:
				case OP_JNZ:
				case OP_JC:
				case OP_JNC:
				case OP_JN:{
					char flag = (r.op == OP_JZ || r.op == OP_JNZ) ? 'Z' : (r.op == OP_JN ? 'N' : 'C');
					int bit = flag == 'Z' ? CPU::FLAG_Z : (flag == 'N' ? CPU::FLAG_N : CPU::FLAG_C);
					bool wantOn = !(r.op == OP_JNZ || r.op == OP_JNC);
					bool isOn = getBit(was.flags, bit);
					bool taken = now.pc == target;
					std::string result = taken ? fmt("jump %s to %s", target < was.pc ? "back" : "ahead", where(target).c_str())
					                           : std::string("no jump, carry on with the next line");
					r.what = fmt("%s = jump if %c is %s. %c is %s, so %s", info.mnemonic, flag, wantOn ? "ON" : "off",
					             flag, isOn ? "ON" : "off", result.c_str());
					break;
				}
				case OP_CALL: r.what = fmt("Call %s: push the return address 0x%04X onto the stack, then jump", where(target).c_str(), (Word)(was.pc + 3)); break;
				case OP_RET:  r.what = fmt("Return: pop 0x%04X off the stack and jump back there", now.pc); break;
				case OP_PUSH: r.what = fmt("Push R%d = %d onto the stack at 0x%04X. SP moves down to 0x%04X", d, x, was.sp, now.sp); break;
				case OP_POP:  r.what = fmt("Pop the top of the stack (0x%04X) into R%d  →  %d. SP moves up", now.sp, d, res); break;
				case OP_PUSHF: r.what = "Push the flags onto the stack"; break;
				case OP_POPF:  r.what = "Pop the flags back off the stack"; break;
				default:      r.what = info.mnemonic; break;
			}

			std::string changed;
			const char* names[4] = {"Z", "C", "N", "V"};
			for(int i = 0; i < 4; ++i){
				bool before = getBit(was.flags, i), after = getBit(now.flags, i);
				if(before != after) changed += fmt("%s%s turned %s", changed.empty() ? "" : ", ", names[i], after ? "ON" : "off");
			}
			r.extra = note + (changed.empty() ? "" : "Flags: " + changed + ".");
		}

		std::string stackTag(Word address){
			int i = owner[address];
			if(i < 0) return "";
			const StepRec& r = hist[i];
			if(r.op == OP_CALL) return r.writes[0].address == address ? "return addr hi" : "return addr lo";
			if(r.op == OP_PUSH) return fmt("saved R%d", r.reg & 7);
			if(r.op == OP_PUSHF) return "saved flags";
			return "";
		}

		Lines programPanel(int height){
			Word pc = board.cpu.getPC();
			int at = -1;
			int last = hist.empty() ? -1 : hist.back().before.pc;
			for(size_t i = 0; i < addrs.size(); ++i) if(addrs[i] == pc) at = (int)i;
			if(at < 0) for(size_t i = 0; i < addrs.size(); ++i) if(addrs[i] == last) at = (int)i;	//after HLT: stay on it
			int first = at < 0 ? 0 : at - 4;
			if(first > (int)addrs.size() - height) first = (int)addrs.size() - height;
			if(first < 0) first = 0;
			bool faulted = board.cpu.getState() == CpuState::Faulted;

			Lines body;
			for(int i = first; i < (int)addrs.size() && (int)body.size() < height; ++i){
				Word a = addrs[i];
				std::string mark = "  ";
				std::string colour = "";
				if(faulted && a == board.cpu.getFault().pc) { mark = RED + "✗ " + RST; colour = RED + BOLD; }
				else if(running() && a == pc)              { mark = YEL + "▶ " + RST; colour = WHITE + BOLD; }
				else if(a == last)                         { mark = GRN + "✓ " + RST; }
				std::string label = labelAt(a);
				body.push_back(mark + GRAY + fmt("%04X ", a) + RST + MAG + pad(label.empty() ? "" : label + ":", 7) + RST +
				               colour + instrText(a) + RST);
			}
			return box("PROGRAM in ROM", 29, body, height);
		}

		Lines registerPanel(int height){
			Snap now = snap(board.cpu);
			Lines body;
			for(int i = 0; i < CPU::NUM_REGS; ++i){
				std::string line = fmt("R%d  0x%02X  %3d", i, now.reg[i], now.reg[i]);
				if(!hist.empty() && hist.back().before.reg[i] != now.reg[i])
					line = YEL + BOLD + line + RST + YEL + fmt(" ◀ was %d", hist.back().before.reg[i]) + RST;
				body.push_back(line);
			}
			body.push_back(GRAY + rep("─", 23) + RST);
			body.push_back(fmt("PC  0x%04X", now.pc) + GRAY + "  next instr" + RST);
			int used = MemoryMap::STACK_END - now.sp;
			body.push_back(fmt("SP  0x%04X", now.sp) + GRAY + (used ? fmt("  %d used", used) : std::string("  empty")) + RST);
			body.push_back(fmt("P   0x%04X", makeWord(now.reg[7], now.reg[6])) + GRAY + "  R6:R7" + RST);
			return box("REGISTERS", 23, body, height);
		}

		Lines stackPanel(int height){
			Word sp = board.cpu.getSP();
			std::vector<Word> used;
			for(int a = MemoryMap::STACK_END; a > sp; --a) used.push_back((Word)a);
			Lines body;
			if(used.empty()) body.push_back(GRAY + "empty: nothing pushed yet" + RST);
			std::vector<int> show;
			if(used.size() <= 8) for(size_t i = 0; i < used.size(); ++i) show.push_back((int)i);
			else{
				show.push_back(0);
				show.push_back(1);
				show.push_back(-1);
				for(size_t i = used.size() - 5; i < used.size(); ++i) show.push_back((int)i);
			}
			for(size_t k = 0; k < show.size(); ++k){
				if(show[k] < 0){ body.push_back(GRAY + fmt("   ⋮   %lu more bytes", (unsigned long)used.size() - 7) + RST); continue; }
				Word a = used[show[k]];
				bool top = a == (Word)(sp + 1);
				body.push_back((top ? YEL + "▸ " : std::string("  ")) + fmt("%04X  %02X  ", a, board.bus.peek(a)) + RST + GRAY + stackTag(a) + RST);
			}
			body.push_back(YEL + fmt("  SP → %04X  next free slot", sp) + RST);
			body.push_back(GRAY + "  (the stack grows down ↓)" + RST);
			if(sc.watch){
				while((int)body.size() < height - 2) body.push_back("");
				Byte v = board.bus.peek(sc.watch);
				bool hit = !hist.empty() && !hist.back().writes.empty() && hist.back().writes[0].address == sc.watch;
				body.push_back(CYAN + BOLD + "MEMORY WATCH" + RST);
				body.push_back((hit ? YEL + BOLD : std::string()) + fmt("RAM 0x%04X = 0x%02X = %d", sc.watch, v, v) + RST + GRAY + "  " + sc.watchName + RST);
			}
			return box("STACK", 34, body, height);
		}

		Lines screenPanel(){
			std::vector<Word> fresh;
			if(!hist.empty()) for(size_t i = 0; i < hist.back().writes.size(); ++i) fresh.push_back(hist.back().writes[i].address);
			Lines body;
			for(int y = 0; y < MemoryMap::SCREEN_HEIGHT; ++y){
				std::string line;
				for(int x = 0; x < MemoryMap::SCREEN_WIDTH; ++x){
					Word a = (Word)(MemoryMap::VRAM_START + y * MemoryMap::SCREEN_WIDTH + x);
					bool isNew = false;
					for(size_t i = 0; i < fresh.size(); ++i) if(fresh[i] == a) isNew = true;
					if(isNew)                 line += YEL + "██";
					else if(board.bus.peek(a)) line += GRN + "██";
					else                      line += GRAY + "· ";
				}
				body.push_back(line + RST);
			}
			return box("VRAM 0xE000 = the 32 × 16 screen the PPU will draw", 64, body, MemoryMap::SCREEN_HEIGHT, false);
		}

		void draw(){
			Snap now = snap(board.cpu);
			Lines out;
			out.push_back(header(sc.title, fmt("step %lu · cycles %lu ", (unsigned long)hist.size(), now.cycles)));
			out.push_back(GRAY + " " + sc.goal + RST);

			if(sc.screen){
				add(out, hjoin({programPanel(MemoryMap::SCREEN_HEIGHT), screenPanel()}));
				std::string regs = " ";
				for(int i = 0; i < CPU::NUM_REGS; ++i){
					bool changed = !hist.empty() && hist.back().before.reg[i] != now.reg[i];
					regs += (changed ? YEL + BOLD : std::string()) + fmt("R%d=%-3d ", i, now.reg[i]) + RST;
				}
				Word p = makeWord(now.reg[7], now.reg[6]);
				regs += GRAY + "  P = R6:R7 = " + RST + fmt("0x%04X", p) + GRAY + pixelNote(p) + RST;
				out.push_back(regs);
				out.push_back(" FLAGS " + lamps(now.flags) + GRAY + fmt("    PC 0x%04X   SP 0x%04X", now.pc, now.sp) + RST);
			}
			else{
				add(out, hjoin({programPanel(13), registerPanel(13), stackPanel(13)}));
				out.push_back(" FLAGS  " + lamps(now.flags));
			}

			Lines what;
			if(hist.empty()){
				what.push_back(GRAY + "Nothing has run yet. The program is in ROM and the CPU is reset." + RST);
				what.push_back(WHITE + BOLD + "▶ marks the next instruction. Press Enter to fetch, decode and execute it." + RST);
			}
			else{
				what.push_back(hist.back().strip);
				what.push_back(WHITE + BOLD + "→ " + hist.back().what + RST);
				what.push_back(YEL + hist.back().extra + RST);
			}
			add(out, box(fmt("WHAT JUST HAPPENED (step %lu)", (unsigned long)hist.size()), 96, what, 3));

			if(board.cpu.getState() == CpuState::Halted && sc.summary)
				out.push_back(bar(ON_GRN + BOLD, " ✓ " + sc.summary(board, (unsigned long)hist.size())));
			else if(board.cpu.getState() == CpuState::Faulted){
				const Fault& f = board.cpu.getFault();
				out.push_back(bar(ON_RED + BOLD, fmt(" ✗ FAULT  %s at PC 0x%04X: %s", faultName(f.type), f.pc, f.message.c_str())));
			}
			else out.push_back("");
			out.push_back(footer("[Enter] step   [Backspace] step back   [R] run (any key pauses)   [S] start over   [Esc] menu"));
			show(out);
		}
};

//------------------------------------------------------------------------------------------------
//SCENE 1: the memory map and the Bus

void sceneMemory(){
	struct Op { const char* text; int region; };
	const Op ops[] = {
		{"bus.write(0x8005, 0x42)",           1},
		{"bus.write(0xE001, 0x07)",           2},
		{"bus.writeWord(0x8005, 0x1234)",     1},
		{"bus.write(0x0003, 0x99)",           0},
		{"bus.read(0xF000)",                  3},
		{"bus.attach(chip at 0x8000)",        1}
	};
	const int count = 6;
	const bool refused[] = {false, false, false, true, true, true};

	Board* b = new Board();
	int done = 0;
	std::string what, extra;

	while(!quitAll){
		//the map, with the chip the last request went to lit up
		int lit = done ? ops[done - 1].region : -1;
		std::string litColour = done && refused[done - 1] ? ON_RED : ON_YEL;
		struct Row { const char* address; const char* chip; int region; const char* use; };
		const Row rows[] = {
			{"0x0000", "┌──────────────┐", -1, ""},
			{"",       "│ ROM    32 KB │",  0, "the program, read-only"},
			{"",       "│              │",  0, ""},
			{"0x8000", "├──────────────┤", -1, ""},
			{"",       "│ RAM    24 KB │",  1, "variables"},
			{"0xE000", "├──────────────┤", -1, ""},
			{"",       "│ VRAM    4 KB │",  2, "screen pixels"},
			{"0xF000", "├──────────────┤", -1, ""},
			{"",       "│ I/O    256 B │",  3, "ports (not on this board)"},
			{"0xF100", "├──────────────┤", -1, ""},
			{"",       "│ STACK 3840 B │",  4, "grows down from 0xFFFF"},
			{"0xFFFF", "└──────────────┘", -1, ""}
		};
		Lines map;
		for(int i = 0; i < 12; ++i){
			std::string chip = rows[i].chip;
			if(rows[i].region >= 0 && rows[i].region == lit){
				std::string inside = chip.substr(3, chip.size() - 6);		//between the two "│" (3 bytes each)
				chip = "│" + litColour + BOLD + inside + RST + "│";
			}
			map.push_back(GRAY + pad(rows[i].address, 7) + RST + chip + "  " + GRAY + rows[i].use + RST);
		}

		Lines list;
		for(int i = 0; i < count; ++i){
			if(i < done)       list.push_back((refused[i] ? RED + "✗ " : GRN + "✓ ") + RST + ops[i].text + (refused[i] ? RED + "  refused" + RST : GRN + "  ok" + RST));
			else if(i == done) list.push_back(YEL + "▶ " + WHITE + BOLD + ops[i].text + RST);
			else               list.push_back(GRAY + "  " + ops[i].text + RST);
		}
		list.push_back("");
		list.push_back(GRAY + "Every address belongs to exactly one" + RST);
		list.push_back(GRAY + "chip. The Bus looks the address up" + RST);
		list.push_back(GRAY + "and passes the request to that chip." + RST);

		Lines out;
		out.push_back(header("Scene 1 · Memory map and Bus", "64 KB, 5 chips "));
		out.push_back(GRAY + " Each request goes over the Bus. Watch which chip it lands in, and which ones get refused." + RST);
		add(out, hjoin({box("64 KB MEMORY MAP", 50, map, 12), box("REQUESTS TO THE BUS", 40, list, 12)}));
		Lines info;
		if(!done) info.push_back(WHITE + BOLD + "Press Enter to send the first request over the Bus." + RST);
		else{
			Lines a = wrap("→ " + what, 96), b2 = wrap(extra, 96);
			for(size_t i = 0; i < a.size(); ++i)  info.push_back(WHITE + BOLD + a[i] + RST);
			for(size_t i = 0; i < b2.size(); ++i) info.push_back(YEL + b2[i] + RST);
		}
		add(out, box("WHAT JUST HAPPENED", 96, info, 3));
		out.push_back(footer(done < count ? "[Enter] next request   [S] start over   [Esc] menu" : "All done.   [S] start over   [Esc] menu"));
		show(out);

		int k = getKey();
		if(k == K_ESC || k == 'q' || k == 'Q' || k == K_EOF) break;
		if(k == 's' || k == 'S'){ delete b; b = new Board(); done = 0; continue; }
		if(k != K_ENTER && k != ' ' && k != K_RIGHT) continue;
		if(done >= count) continue;

		switch(done){
			case 0:
				b->bus.write(0x8005, 0x42);
				what  = "The Bus sees 0x8005 is in 0x8000-0xDFFF, so it sends it to RAM, offset 5 (0x8005 − 0x8000).";
				extra = fmt("RAM offset 5 now holds 0x%02X.", b->ram.read(5));
				break;
			case 1:
				b->bus.write(0xE001, 0x07);
				what  = "0xE001 is in 0xE000-0xEFFF, so it goes to the VRAM chip, offset 1. That is pixel (1, 0).";
				extra = fmt("VRAM offset 1 now holds 0x%02X. The PPU will draw this pixel.", b->vram.read(1));
				break;
			case 2:
				b->bus.writeWord(0x8005, 0x1234);
				what  = "16 bits need two bytes, low byte first (little-endian): 0x34 goes to 0x8005, 0x12 to 0x8006.";
				extra = fmt("RAM[5] = 0x%02X, RAM[6] = 0x%02X, and readWord(0x8005) gives back 0x%04X.", b->ram.read(5), b->ram.read(6), b->bus.readWord(0x8005));
				break;
			case 3:
				b->bus.write(0x0003, 0x99);
				what  = fmt("0x0003 is in ROM, and ROM refuses writes → %s: %s.", faultName(b->bus.getFault().type), b->bus.getFault().message.c_str());
				extra = "The Bus reports the fault instead of crashing, so a buggy program can never overwrite itself.";
				b->bus.clearFault();
				break;
			case 4:{
				Byte v = b->bus.read(0xF000);
				what  = fmt("Nothing is attached at 0xF000 on this board → %s. The read gives 0x%02X.", faultName(b->bus.getFault().type), v);
				extra = "On the full Console the I/O ports sit here (scene 7).";
				b->bus.clearFault();
				break;
			}
			case 5:{
				static Ram extraChip(0x100, "EXTRA");
				bool ok = b->bus.attach(0x8000, 0x80FF, &extraChip);
				what  = ok ? "attach accepted (this should not happen)" : "attach refused: 0x8000-0x80FF already belongs to RAM.";
				extra = "Two chips can never answer the same address, so every read has exactly one answer.";
				break;
			}
		}
		done++;
	}
	delete b;
}

//------------------------------------------------------------------------------------------------
//SCENE 3: flags. Type a sum, the real CPU works it out, the lamps say what happened.

void sceneFlags(){
	std::string input = "200 + 100";
	std::string error;
	int a = 200, b = 100;
	char op = '+';

	while(!quitAll){
		//run it on the CPU: LDI R0, a ; LDI R1, b ; ADD/SUB R0, R1 ; HLT
		Board* m = new Board();
		Byte ua = (Byte)a, ub = (Byte)b;
		m->load({OP_LDI, 0x00, ua, OP_LDI, 0x01, ub, (Byte)(op == '+' ? OP_ADD : OP_SUB), 0x01, OP_HLT});
		m->cpu.run(10);
		Byte r = m->cpu.getReg(0);
		Byte flags = m->cpu.getFlags();
		std::string ran;
		Word address = 0;
		for(int i = 0; i < 4; ++i){
			Byte length = 0;
			ran += (i ? GRAY + "  ·  " + RST : std::string()) + m->dis.line(address, length).substr(8);
			address = (Word)(address + length);
		}
		delete m;

		int sa = (signed char)ua, sb = (signed char)ub, sr = (signed char)r;
		int trueU = op == '+' ? ua + ub : ua - ub;
		int trueS = op == '+' ? sa + sb : sa - sb;

		Lines sum;
		sum.push_back(GRAY + "            unsigned   signed    hex     binary" + RST);
		sum.push_back(fmt("   A          %3d      %4d     0x%02X    ", ua, sa, ua) + bin(ua));
		sum.push_back(fmt(" %c B          %3d      %4d     0x%02X    ", op, ub, sb, ub) + bin(ub));
		sum.push_back(GRAY + " " + rep("─", 52) + RST);
		sum.push_back(BOLD + WHITE + fmt(" = R0         %3d      %4d     0x%02X    ", r, sr, r) + bin(r) + RST);

		bool z = getBit(flags, CPU::FLAG_Z), c = getBit(flags, CPU::FLAG_C), n = getBit(flags, CPU::FLAG_N), v = getBit(flags, CPU::FLAG_V);
		std::string whyZ = z ? "the answer is exactly 0" : "the answer is not 0";
		std::string whyC = op == '+'
			? (trueU > 255 ? fmt("%d + %d = %d is over 255: it wrapped to %d and the extra bit went into C", ua, ub, trueU, r)
			               : fmt("%d + %d = %d fits in 8 bits (0 to 255), nothing carried out", ua, ub, trueU))
			: (trueU < 0   ? fmt("%d is smaller than %d, so the CPU had to borrow", ua, ub)
			               : fmt("%d is not smaller than %d, no borrow needed", ua, ub));
		std::string whyN = n ? fmt("bit 7 of the answer is 1, so read as signed it is negative (%d)", sr)
		                     : "bit 7 of the answer is 0, so it is not negative";
		std::string whyV = (trueS < -128 || trueS > 127)
			? fmt("signed: %d %c %d = %d is outside -128 to 127, so it wrapped to %d", sa, op, sb, trueS, sr)
			: fmt("signed: %d %c %d = %d fits in -128 to 127", sa, op, sb, trueS);

		Lines lights;
		lights.push_back(lamp("Z zero    ", z) + "  " + (z ? WHITE : GRAY) + whyZ + RST);
		lights.push_back(lamp("C carry   ", c) + "  " + (c ? WHITE : GRAY) + whyC + RST);
		lights.push_back(lamp("N negative", n) + "  " + (n ? WHITE : GRAY) + whyN + RST);
		lights.push_back(lamp("V overflow", v) + "  " + (v ? WHITE : GRAY) + whyV + RST);

		Lines out;
		out.push_back(header("Scene 3 · Flags", "the CPU's four yes/no lamps "));
		out.push_back(GRAY + " After every sum the CPU sets four flags. Jumps like JZ and JC read them to make decisions." + RST);
		out.push_back("");
		out.push_back(" Type a sum and press Enter:  " + YEL + BOLD + input + RST + YEL + "_" + RST + "   " + RED + error + RST);
		out.push_back(GRAY + " -128 to 255, + or -.   Try: [a] 255 + 1   [b] 127 + 1   [c] 5 - 5   [d] 0 - 1   [e] -128 - 1" + RST);
		add(out, box("THE SUM, WORKED OUT BY THE REAL CPU", 96, sum, 5));
		add(out, box("FLAGS", 96, lights, 4));
		out.push_back(GRAY + " The CPU ran:  " + RST + ran);
		out.push_back("");
		out.push_back(footer("[type] a sum   [Enter] run it   [a-e] examples   [Esc] menu"));
		show(out);

		int k = getKey();
		if(k == K_ESC || k == 'q' || k == 'Q' || k == K_EOF) break;
		const char* examples[] = {"255 + 1", "127 + 1", "5 - 5", "0 - 1", "-128 - 1"};
		if(k >= 'a' && k <= 'e'){ input = examples[k - 'a']; k = K_ENTER; }
		if(k == K_BACK){ if(!input.empty()) input.erase(input.size() - 1); continue; }
		if((k >= '0' && k <= '9') || k == ' ' || k == '+' || k == '-'){
			if(input.size() < 12) input += (char)k;
			continue;
		}
		if(k != K_ENTER) continue;

		//read "A op B"
		int na = 0, nb = 0;
		char o = 0;
		char extraChar = 0;
		if(sscanf(input.c_str(), " %d %c %d %c", &na, &o, &nb, &extraChar) != 3 || (o != '+' && o != '-')){
			error = "write it like 200 + 100";
			continue;
		}
		if(na < -128 || na > 255 || nb < -128 || nb > 255){
			error = "8 bits only: use -128 to 255";
			continue;
		}
		error = "";
		a = na; b = nb; op = o;
	}
}

//------------------------------------------------------------------------------------------------
//SCENE 5: faults. Pick a broken program, step it, watch the CPU catch it.

void sceneFaults(){
	struct Case { const char* what; const char* fault; std::vector<Byte> bytes; };
	const Case cases[] = {
		{"Byte 0xFF is not an instruction",    "InvalidOpcode",     {0x00, 0xFF}},
		{"LDI R9, 1: only R0-R7 exist",        "InvalidRegister",   {0x11, 0x09, 0x01}},
		{"ST R0, [0x0010]: write into ROM",    "IllegalWrite",      {0x13, 0x00, 0x10, 0x00}},
		{"LD R0, [0xF000]: no chip there",     "UnmappedAccess",    {0x12, 0x00, 0x00, 0xF0}},
		{"POP R0 on an empty stack",           "StackUnderflow",    {0x81, 0x00}},
		{"JMP 0x9000: outside the program",    "InvalidJumpTarget", {0x60, 0x00, 0x90}},
		{"PUSH R0 forever (press R to run it)", "StackOverflow",    {0x80, 0x00, 0x60, 0x00, 0x00}}
	};

	while(!quitAll){
		Lines list;
		for(int i = 0; i < 7; ++i)
			list.push_back(YEL + BOLD + fmt("%d", i + 1) + RST + "   " + pad(cases[i].what, 40) + RED + cases[i].fault + RST);
		Lines out;
		out.push_back(header("Scene 5 · Faults", "7 kinds of mistakes "));
		out.push_back(GRAY + " A broken program must never crash the emulator. The CPU stops, names the mistake, and PC" + RST);
		out.push_back(GRAY + " stays on the bad instruction. Pick one, step it, and watch it get caught." + RST);
		out.push_back("");
		add(out, box("PICK A BROKEN PROGRAM", 96, list, 7));
		out.push_back("");
		out.push_back(footer("[1-7] open it   [Esc] menu"));
		show(out);

		int k = getKey();
		if(k == K_ESC || k == 'q' || k == 'Q' || k == K_EOF) return;
		if(k < '1' || k > '7') continue;
		const Case& c = cases[k - '1'];

		Scene sc;
		sc.title = fmt("Scene 5 · Fault: %s", c.what);
		sc.goal = fmt("Step it and watch the CPU catch the mistake (%s).", c.fault);
		sc.program = c.bytes;
		if(k == '7'){
			sc.labels = {{0x0000, "again"}};
			sc.delayMs = 60;
			sc.slowSteps = 20;			//then flat out: it takes 3840 pushes to fill the stack
		}
		Stepper(sc).run();
	}
}

//------------------------------------------------------------------------------------------------
//SCENE 7: I/O ports on the full Console: buttons, frames + random numbers, text out

void sceneIo(){
	Console buttons, frames, text;
	const Byte readInput[] = {0x12, 0x00, 0x00, 0xF0, 0x01};		//LD R0, [0xF000] ; HLT
	const Byte game[] = {
		0x12, 0x01, 0x03, 0xF0,		//0x0000 LD R1, [0xF003]   frame now
		0x12, 0x00, 0x03, 0xF0,		//0x0004 LD R0, [0xF003]   wait:
		0x37, 0x01,					//0x0008 CMP R0, R1
		0x61, 0x04, 0x00,			//0x000A JZ 0x0004         same frame: keep waiting
		0x12, 0x02, 0x01, 0xF0,		//0x000D LD R2, [0xF001]   random byte
		0x13, 0x02, 0x00, 0x80,		//0x0011 ST R2, [0x8000]   save it in RAM
		0x60, 0x00, 0x00			//0x0015 JMP 0x0000
	};
	const Byte hello[] = {
		0x11, 0x00, 0x48, 0x13, 0x00, 0x04, 0xF0,		//LDI R0, 'H' ; ST R0, [0xF004]
		0x11, 0x00, 0x49, 0x13, 0x00, 0x04, 0xF0,		//LDI R0, 'I' ; ST R0, [0xF004]
		0x11, 0x00, 0x20, 0x13, 0x00, 0x04, 0xF0,		//LDI R0, ' ' ; ST R0, [0xF004]
		0x01											//HLT
	};
	buttons.loadProgram(readInput, sizeof(readInput));
	buttons.runFrame();
	frames.loadProgram(game, sizeof(game));
	text.loadProgram(hello, sizeof(hello));
	std::string history;
	int framesRun = 0;

	while(!quitAll){
		Byte held = buttons.io().getInput();
		const char* names[8] = {"SELECT", "START", "B", "A", "RIGHT", "LEFT", "DOWN", "UP"};		//bit 7 first
		std::string nameRow, bitRow, valueRow;
		for(int i = 0; i < 8; ++i){
			int bit = 7 - i;
			bool on = getBit(held, bit);
			nameRow  += (on ? WHITE + BOLD : GRAY) + pad(names[i], 7) + RST;
			bitRow   += GRAY + pad(fmt("bit %d", bit), 7) + RST;
			valueRow += (on ? ON_GRN + "  1  " + RST : GRAY + "  0  " + RST) + "  ";
		}
		Lines pad1;
		pad1.push_back(GRAY + "Press to hold/release:  arrows   Z = A   X = B   S   D" + RST);
		pad1.push_back("");
		pad1.push_back(nameRow);
		pad1.push_back(bitRow);
		pad1.push_back(valueRow);
		pad1.push_back("");
		pad1.push_back(GRAY + "The CPU ran  " + RST + "LD R0, [0xF000]" + GRAY + "  →  " + RST + BOLD + WHITE +
		               fmt("R0 = 0x%02X = ", buttons.cpu().getReg(0)) + bin(buttons.cpu().getReg(0)) + RST);

		Lines pad2;
		pad2.push_back(GRAY + "Press T: the program writes" + RST);
		pad2.push_back(GRAY + "'H', 'I', ' ' into port 0xF004." + RST);
		pad2.push_back("");
		pad2.push_back(GRAY + "What it printed:" + RST);
		std::string printed = text.io().getConsoleText();
		if(printed.size() > 30) printed = printed.substr(printed.size() - 30);
		pad2.push_back(YEL + BOLD + printed + RST);

		Lines pad3;
		pad3.push_back(GRAY + "Press F to run one frame. The program waits for the FRAME port to change (like a game waiting" + RST);
		pad3.push_back(GRAY + "for the next screen refresh), then reads a random byte from RNG and saves it to RAM 0x8000." + RST);
		pad3.push_back("");
		if(framesRun < 2) pad3.push_back(fmt("frame %d", frames.io().getFrame()) + GRAY + "   (the first frame only starts the wait, nothing saved yet)" + RST);
		else pad3.push_back(fmt("frame %d", frames.io().getFrame()) + GRAY + "   random byte saved at 0x8000 = " + RST + BOLD + YEL + fmt("0x%02X", frames.bus().peek(0x8000)) + RST);
		pad3.push_back(GRAY + "earlier: " + RST + history);

		Lines out;
		out.push_back(header("Scene 7 · I/O ports", "the full Console, ports at 0xF000 "));
		out.push_back(GRAY + " Programs talk to the outside world by reading and writing special addresses: the I/O ports." + RST);
		add(out, hjoin({box("BUTTONS → port 0xF000 INPUT", 56, pad1, 7), box("TEXT OUT → port 0xF004", 35, pad2, 7)}));
		add(out, box("FRAMES + RANDOM → ports 0xF003 FRAME, 0xF001 RNG", 96, pad3, 5));
		out.push_back(GRAY + " Team hooks:  " + RST + "Arnav" + GRAY + " runFrame()  ·  " + RST + "Ashmit" + GRAY +
		              " pixel(), setButton()  ·  " + RST + "Piyush" + GRAY + " write listener, poke()" + RST);
		out.push_back("");
		out.push_back(footer("[arrows Z X S D] buttons   [F] next frame   [T] print text   [Esc] menu"));
		show(out);

		int k = getKey();
		if(k == K_ESC || k == 'q' || k == 'Q' || k == K_EOF) return;
		int button = -1;
		switch(k){
			case K_UP:    button = IoPorts::BTN_UP; break;
			case K_DOWN:  button = IoPorts::BTN_DOWN; break;
			case K_LEFT:  button = IoPorts::BTN_LEFT; break;
			case K_RIGHT: button = IoPorts::BTN_RIGHT; break;
			case 'z': case 'Z': button = IoPorts::BTN_A; break;
			case 'x': case 'X': button = IoPorts::BTN_B; break;
			case 's': case 'S': button = IoPorts::BTN_START; break;
			case 'd': case 'D': button = IoPorts::BTN_SELECT; break;
		}
		if(button >= 0){
			buttons.io().setButton((IoPorts::Button)button, !getBit(held, button));
			buttons.cpu().reset();		//run LD R0, [0xF000] again with the new buttons
			buttons.runFrame();
		}
		else if(k == 'f' || k == 'F'){
			if(framesRun >= 2){
				history += fmt("%02X ", frames.bus().peek(0x8000));
				if(history.size() > 60) history = history.substr(history.size() - 60);
			}
			frames.runFrame();
			framesRun++;
		}
		else if(k == 't' || k == 'T'){
			text.cpu().reset();
			text.runFrame();
		}
	}
}

//------------------------------------------------------------------------------------------------
//SCENES 2, 4, 6: programs on the dashboard

Scene loopScene(){
	Scene sc;
	sc.title = "Scene 2 · Fetch → Decode → Execute";
	sc.goal = "Add 10 + 9 + ... + 1. Watch R1 count down, R0 grow, and JNZ jump back to loop until R1 hits 0.";
	sc.program = {
		0x11, 0x00, 0x00,			//LDI R0, 0
		0x11, 0x01, 0x0A,			//LDI R1, 10
		0x30, 0x01,					//ADD R0, R1      <- loop
		0x41, 0x01,					//DEC R1
		0x62, 0x06, 0x00,			//JNZ 0x0006      -> back to loop while R1 != 0
		0x13, 0x00, 0x00, 0x80,		//ST R0, [0x8000]
		0x01						//HLT
	};
	sc.labels = {{0x0000, "start"}, {0x0006, "loop"}, {0x000D, "save"}};
	sc.watch = 0x8000;
	sc.watchName = "the answer";
	sc.delayMs = 250;
	sc.summary = [](Board& b, unsigned long steps){
		return fmt("Finished: R0 = %d = 10+9+...+1, saved in RAM[0x8000] = %d. %lu instructions, %lu cycles.",
		           b.cpu.getReg(0), b.bus.peek(0x8000), steps, b.cpu.getCycles());
	};
	return sc;
}

Scene callScene(){
	Scene sc;
	sc.title = "Scene 4 · Stack, CALL and RET";
	sc.goal = "A subroutine doubles R0. Watch CALL push the return address onto the stack and RET pop it off.";
	sc.program = {
		0x11, 0x00, 0x05,			//0x0000 LDI R0, 5
		0x66, 0x0A, 0x00,			//0x0003 CALL 0x000A    (subroutine: double R0)
		0x11, 0x02, 0x01,			//0x0006 LDI R2, 1       <- RET comes back here
		0x01,						//0x0009 HLT
		0x80, 0x00,					//0x000A PUSH R0         subroutine starts
		0x81, 0x01,					//0x000C POP R1
		0x30, 0x01,					//0x000E ADD R0, R1
		0x67						//0x0010 RET
	};
	sc.labels = {{0x0000, "main"}, {0x0006, "back"}, {0x000A, "twice"}};
	sc.delayMs = 500;
	sc.summary = [](Board& b, unsigned long){
		return fmt("Finished: R0 = %d (5 doubled), back in main (R2 = %d), SP = 0x%04X so the stack is empty again.",
		           b.cpu.getReg(0), b.cpu.getReg(2), b.cpu.getSP());
	};
	return sc;
}

Scene drawScene(){
	Scene sc;
	sc.title = "Scene 6 · Graphics";
	sc.goal = "The CPU draws an X by writing into VRAM. Press R to watch it draw, or Enter for one step.";
	sc.program = {
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
	sc.labels = {{0x000F, "top"}, {0x0020, "bot"}, {0x0031, "diag1"}, {0x0046, "diag2"}};
	sc.screen = true;
	sc.delayMs = 12;
	sc.summary = [](Board& b, unsigned long steps){
		return fmt("Finished: %lu instructions, %lu cycles. Ashmit's PPU will draw these same VRAM bytes.",
		           steps, b.cpu.getCycles());
	};
	return sc;
}

//------------------------------------------------------------------------------------------------

Lines banner(){
	const char* rows[5] = {
		"███   ███  █   █  ████  █           ██ ",
		"█  █   █    █ █   █     █          █  █",
		"███    █     █    ███   █     ███   ██ ",
		"█      █    █ █   █     █          █  █",
		"█     ███  █   █  ████  ████        ██ "
	};
	const std::string colours[5] = {"\x1b[96m", "\x1b[96m", "\x1b[94m", "\x1b[95m", "\x1b[95m"};
	Lines out;
	for(int i = 0; i < 5; ++i) out.push_back("     " + colours[i] + BOLD + rows[i] + RST);
	return out;
}

int main(){
	if(!setupTerminal()){
		printf("This terminal cannot show colours. Use Windows Terminal or PowerShell, or run the old demo: mingw32-make demo\n");
		return 1;
	}

	while(!quitAll){
		Lines out;
		out.push_back("");
		add(out, banner());
		out.push_back("");
		out.push_back(WHITE + BOLD + "     An 8-bit computer built in C++" + RST + GRAY + "  ·  CPU architecture and memory by Aashish Singh" + RST);
		out.push_back(GRAY + "     38 instructions · 8 registers · 4 flags · 64 KB memory · 7 fault types · 158 automatic tests" + RST);
		out.push_back("");
		Lines menu;
		const char* items[7][2] = {
			{"Memory map and Bus",          "where every address goes, and what gets refused"},
			{"Fetch → Decode → Execute",    "step through a loop that adds 10 + 9 + ... + 1"},
			{"Flags",                       "type any sum, watch Z C N V light up"},
			{"Stack, CALL and RET",         "a subroutine call, step by step"},
			{"Faults",                      "broken programs are caught, never crash"},
			{"Graphics",                    "the CPU draws an X into video memory"},
			{"I/O ports",                   "buttons, frames, random numbers, text"}
		};
		for(int i = 0; i < 7; ++i)
			menu.push_back(YEL + BOLD + fmt(" %d ", i + 1) + RST + "  " + WHITE + BOLD + pad(items[i][0], 28) + RST + GRAY + items[i][1] + RST);
		menu.push_back("");
		menu.push_back(YEL + BOLD + " Q " + RST + "  " + GRAY + "quit" + RST);
		add(out, box("PICK A SCENE", 80, menu, 9));
		int width = terminalWidth();
		if(width < SCREEN_W)
			out.push_back(YEL + fmt(" This window is %d columns wide. Make it at least %d (maximise it, or Ctrl + minus).", width, SCREEN_W) + RST);
		show(out);

		int k = getKey();
		if(k == 'q' || k == 'Q' || k == K_ESC || k == K_EOF) break;
		switch(k){
			case '1': sceneMemory(); break;
			case '2': { Scene sc = loopScene(); Stepper(sc).run(); break; }
			case '3': sceneFlags(); break;
			case '4': { Scene sc = callScene(); Stepper(sc).run(); break; }
			case '5': sceneFaults(); break;
			case '6': { Scene sc = drawScene(); Stepper(sc).run(); break; }
			case '7': sceneIo(); break;
		}
	}
	return 0;
}
