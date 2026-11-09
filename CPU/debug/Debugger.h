//THE STEP DEBUGGER: LOAD ANY .asm, STEP FORWARDS AND BACKWARDS, SAVE THE STEPS AS CSV
#ifndef DEBUGGER_H
#define DEBUGGER_H
#include <deque>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "../Aashish/src/Console.h"
#include "../Aashish/src/CpuSnapshot.h"
#include "../src/SystemState.h"

//It prints nothing, so a terminal (demo/debug.cpp) or a GUI can sit on top of it.
//Undo works like the Stepper in live.cpp: before each step save the whole CPU, record the old
//value of every memory write through the Bus write listener, and put both back on back().
class Debugger{
	public:
		static const unsigned long MAX_STEPS = 100000;	//history kept, and steps per run()

		Debugger();
		Debugger(const Debugger&) = delete;				//the write listener points at this object
		Debugger& operator=(const Debugger&) = delete;

		bool load(const std::string& file);				//parse, check, assemble, load into ROM, reset; false on error
		const std::vector<std::string>& errors() const;	//"line N: message", or why the file could not be read

		StepResult step();								//one instruction, recorded; does nothing after HLT or a fault
		bool back();									//undo the last step: memory, then the whole CPU; false if none
		StepResult run(unsigned long maxSteps = MAX_STEPS);	//until HLT, a fault, or maxSteps (then Ok)

		int currentLine();						//source line of the next instruction, 0 if none
		std::string sourceText(int line) const;			//text of a source line (1 = first), "" outside the file
		unsigned long steps() const;					//steps recorded (at most MAX_STEPS)

		//one row per recorded step, values after the step:
		//step,pc,line,instruction,R0..R7,Z,C,N,V,SP,cycles
		bool writeCsv(const std::string& file) const;

		Console& console();

	private:
		struct Record{
			unsigned long number;						//1 = first step since load, keeps counting after old ones drop
			Word pc;									//where the instruction started
			std::string text;							//disassembled, e.g. "ADD R0, R1"
			CpuSnapshot before;
			Word rngBefore;								//reading or seeding the RNG port changes it outside memory
			std::vector<std::pair<Word, Byte> > writes;	//address, old value, in the order they happened
			SystemState after;
		};

		Console machine;
		std::deque<Record> history;
		std::vector<std::pair<Word, Byte> >* recording;	//where the listener writes while a step runs
		std::map<Word, int> lineAt;
		std::vector<std::string> source;
		std::vector<std::string> problems;
		unsigned long nextNumber;
};
#endif
