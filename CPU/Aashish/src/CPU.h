//THE BRAIN: FETCH, DECODE, EXECUTE

#ifndef CPU_H
#define CPU_H

#include <iosfwd>
#include "Bits.h"
#include "Bus.h"
#include "Fault.h"
#include "Opcodes.h"
#include "InstructionTrace.h"

enum class CpuState   { Running, Halted, Faulted };
enum class StepResult { Ok, Halt, Fault };

class CPU{
	//a save point copies the private parts directly, the fault included, which no public setter can put back
	friend class CpuSnapshot;

	//std::cout << cpu: a register dump on one line. Not a member, because the stream is on the left of <<
	friend std::ostream& operator<<(std::ostream& out, const CPU& cpu);

	public:
		static const int NUM_REGS = 8;
		enum Flag { FLAG_Z = 0, FLAG_C = 1, FLAG_N = 2, FLAG_V = 3 };	//bit positions

		explicit CPU(Bus& bus);

		void reset(Word startAddress = 0x0000);
		StepResult step();							//one instruction
		StepResult run(unsigned long maxSteps);		//until HLT, fault, or limit

		//view for the debugger and tests
		Byte  getReg(int index) const;
		void  setReg(int index, Byte value);
		Word  getPC() const;
		Word  getSP() const;
		Word  getP() const;							//R6:R7 as one 16-bit pointer
		Byte  getFlags() const;
		bool  getFlag(Flag flag) const;
		CpuState getState() const;
		const Fault& getFault() const;
		unsigned long getCycles() const;
		const InstructionTrace& getTrace() const;	//the last 16 instructions: how did we get here?

		//for the State Manager (undo) and the debugger: put a saved state back
		void  setPC(Word value);
		void  setSP(Word value);
		void  setFlags(Byte value);					//only bits 0-3 are kept
		void  setCycles(unsigned long value);
		void  setState(CpuState value);				//Running or Halted also clears the fault

	private:
		Bus&  bus;
		Byte  reg[NUM_REGS];
		Word  pc;
		Word  sp;
		Byte  flags;
		CpuState state;
		Fault fault;
		unsigned long cycles;
		Word  instrStart;							//PC where the current instruction began
		InstructionTrace trace;

		Byte fetchByte();
		Word fetchWord();
		void execute(Byte opcode, const OpcodeInfo& info);

		void setFlag(Flag flag, bool on);
		void setZN(Byte result);
		Byte add(Byte a, Byte b, bool carryIn);
		Byte sub(Byte a, Byte b, bool borrowIn);

		bool push(Byte value);
		bool pop(Byte& value);

		bool checkReg(int index);
		bool checkJump(Word target);
		bool busFailed();
		void raise(FaultType type, Word address, const std::string& message);
};

#endif
