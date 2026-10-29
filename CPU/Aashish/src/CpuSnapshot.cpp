#include "CpuSnapshot.h"
#include "MemoryMap.h"

//the same values CPU::reset(0x0000) sets
CpuSnapshot::CpuSnapshot()
	: pc(0x0000), sp(MemoryMap::STACK_END), flags(0), state(CpuState::Running),
	  fault(), cycles(0), instrStart(0x0000), trace(){
	for(int i = 0; i < CPU::NUM_REGS; ++i) reg[i] = 0;
}

CpuSnapshot::CpuSnapshot(const CPU& cpu)
	: pc(cpu.pc), sp(cpu.sp), flags(cpu.flags), state(cpu.state),
	  fault(cpu.fault), cycles(cpu.cycles), instrStart(cpu.instrStart), trace(cpu.trace){
	for(int i = 0; i < CPU::NUM_REGS; ++i) reg[i] = cpu.reg[i];
}

void CpuSnapshot::restore(CPU& cpu) const{
	for(int i = 0; i < CPU::NUM_REGS; ++i) cpu.reg[i] = reg[i];
	cpu.pc = pc;
	cpu.sp = sp;
	cpu.flags = flags;
	cpu.state = state;
	cpu.fault = fault;
	cpu.cycles = cycles;
	cpu.instrStart = instrStart;
	cpu.trace = trace;
}
