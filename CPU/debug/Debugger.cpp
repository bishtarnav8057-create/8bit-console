#include "Debugger.h"
#include <cstdio>
#include <exception>
#include <fstream>
#include "../Aashish/src/Disassembler.h"
#include "../Aashish/src/MemoryMap.h"
#include "../src/Assembler.h"

const unsigned long Debugger::MAX_STEPS;		//the value is in Debugger.h; this gives it an address

namespace{
	//"line 7: ..." (std::to_string is missing from the old MinGW the team uses)
	std::string lineError(const OperandCheckResult& e){
		char text[24];
		snprintf(text, sizeof(text), "line %d: ", e.lineNumber);
		return text + e.message;
	}
}

Debugger::Debugger() : recording(nullptr), nextNumber(1){
	machine.bus().setWriteListener([this](Word address, Byte oldValue, Byte){
		if(recording) recording->push_back(std::make_pair(address, oldValue));
	});
}

bool Debugger::load(const std::string& file){
	history.clear();
	lineAt.clear();
	source.clear();
	problems.clear();
	nextNumber = 1;
	machine.reset();

	AssembleResult result;
	try{
		result = Assembler().assembleFile(file);
	}
	catch(const std::exception& e){				//the Parser throws when the file cannot be opened
		problems.push_back(e.what());
		return false;
	}
	for(size_t i = 0; i < result.errors.size(); ++i)
		problems.push_back(lineError(result.errors[i]));
	if(!result.ok){
		if(problems.empty()) problems.push_back("the program could not be assembled");
		return false;
	}
	if(!machine.loadProgram(result.bytes)){
		problems.push_back("the program is bigger than ROM");
		return false;
	}

	lineAt = result.lineAt;
	std::ifstream in(file);
	std::string line;
	while(std::getline(in, line)){
		if(!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);
		source.push_back(line);
	}
	return true;
}

const std::vector<std::string>& Debugger::errors() const{
	return problems;
}

StepResult Debugger::step(){
	CPU& cpu = machine.cpu();
	if(cpu.getState() == CpuState::Halted)  return StepResult::Halt;
	if(cpu.getState() == CpuState::Faulted) return StepResult::Fault;

	Record r;
	r.number = nextNumber++;
	r.pc = cpu.getPC();
	Byte length = 0;
	r.text = Disassembler(machine.bus()).line(r.pc, length).substr(8);	//drop "0x0003  "
	r.before = CpuSnapshot(cpu);
	r.rngBefore = machine.io().getRandomState();

	recording = &r.writes;
	StepResult result = machine.step();
	recording = nullptr;

	r.after = captureState(cpu);
	history.push_back(r);
	if(history.size() > MAX_STEPS) history.pop_front();
	return result;
}

bool Debugger::back(){
	if(history.empty()) return false;
	const Record& r = history.back();
	for(size_t i = r.writes.size(); i-- > 0; ){		//newest first, so the oldest value ends up in place
		Word address = r.writes[i].first;
		//writing these ports has side effects (new seed, one more printed character), so poking
		//the "old value" back would not undo them: the RNG is restored below, printed text stays
		if(address == MemoryMap::PORT_RNG || address == MemoryMap::PORT_CONSOLE_OUT) continue;
		machine.bus().poke(address, r.writes[i].second);
	}
	machine.io().seedRandom(r.rngBefore);
	r.before.restore(machine.cpu());
	machine.bus().clearFault();
	nextNumber = r.number;
	history.pop_back();
	return true;
}

StepResult Debugger::run(unsigned long maxSteps){
	StepResult result = StepResult::Ok;
	for(unsigned long i = 0; i < maxSteps && result == StepResult::Ok; ++i) result = step();
	return result;
}

int Debugger::currentLine(){
	std::map<Word, int>::const_iterator it = lineAt.find(machine.cpu().getPC());
	return it == lineAt.end() ? 0 : it->second;
}

std::string Debugger::sourceText(int line) const{
	return (line >= 1 && line <= (int)source.size()) ? source[line - 1] : "";
}

unsigned long Debugger::steps() const{
	return history.size();
}

bool Debugger::writeCsv(const std::string& file) const{
	std::ofstream out(file);
	if(!out) return false;
	out << "step,pc,line,instruction,R0,R1,R2,R3,R4,R5,R6,R7,Z,C,N,V,SP,cycles\n";
	for(size_t i = 0; i < history.size(); ++i){
		const Record& r = history[i];
		const SystemState& s = r.after;
		std::map<Word, int>::const_iterator it = lineAt.find(r.pc);
		char hex[8];
		snprintf(hex, sizeof(hex), "0x%04X", (unsigned int)r.pc);
		out << r.number << ',' << hex << ',' << (it == lineAt.end() ? 0 : it->second) << ",\"" << r.text << '"';
		for(int k = 0; k < CPU::NUM_REGS; ++k) out << ',' << (int)s.registers[k];
		snprintf(hex, sizeof(hex), "0x%04X", (unsigned int)s.stackPointer);
		out << ',' << s.zeroFlag << ',' << s.carryFlag << ',' << s.negativeFlag << ',' << s.overflowFlag
		    << ',' << hex << ',' << s.cycles << '\n';
	}
	return (bool)out;
}

Console& Debugger::console(){
	return machine;
}
