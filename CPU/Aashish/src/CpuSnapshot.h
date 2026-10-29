//A SAVE POINT FOR THE WHOLE CPU: TAKE ONE, RUN ON, PUT IT BACK

#ifndef CPUSNAPSHOT_H
#define CPUSNAPSHOT_H

#include "CPU.h"

//CpuSnapshot is a friend of CPU, so it copies the private members straight in and out: registers,
//PC, SP, flags, state, the fault with all its details, cycles and the instruction trace.
//The public setters cannot do all of that (setState can only keep or clear a fault, never put one back),
//and adding a public setter for every member would let ANY code change them. A friend class gives
//that access to this one trusted class only.
//Memory is not part of it: undo memory with Bus::poke and the write listener's old values.
class CpuSnapshot{
	public:
		CpuSnapshot();							//a CPU straight after reset()
		explicit CpuSnapshot(const CPU& cpu);	//copies everything inside this CPU

		void restore(CPU& cpu) const;			//puts all of it back

	private:
		Byte  reg[CPU::NUM_REGS];
		Word  pc;
		Word  sp;
		Byte  flags;
		CpuState state;
		Fault fault;
		unsigned long cycles;
		Word  instrStart;
		InstructionTrace trace;
};

#endif
