#include "CPU.h"
#include "MemoryMap.h"
#include <cstdio>

namespace{
	std::string hex8(Byte value){
		char text[8];
		snprintf(text, sizeof(text), "0x%02X", (unsigned int)value);
		return text;
	}

	std::string hex16(Word value){
		char text[8];
		snprintf(text, sizeof(text), "0x%04X", (unsigned int)value);
		return text;
	}
}

CPU::CPU(Bus& theBus) : bus(theBus){
	reset(0x0000);
}

void CPU::reset(Word startAddress){
	for(int i = 0; i < NUM_REGS; ++i) reg[i] = 0;
	pc = startAddress;
	sp = MemoryMap::STACK_END;			//empty stack
	flags = 0;
	state = CpuState::Running;
	fault = Fault();
	cycles = 0;
	instrStart = startAddress;
	bus.clearFault();
}

StepResult CPU::step(){
	if(state == CpuState::Halted) return StepResult::Halt;
	if(state == CpuState::Faulted) return StepResult::Fault;

	bus.clearFault();					//only faults caused by THIS instruction count
	instrStart = pc;

	//FETCH
	Byte opcode = fetchByte();
	if(busFailed()) return StepResult::Fault;

	//DECODE
	const OpcodeInfo& info = OpcodeTable::lookup(opcode);
	if(!info.valid){
		raise(FaultType::InvalidOpcode, instrStart, "Invalid opcode " + hex8(opcode));
		return StepResult::Fault;
	}

	//EXECUTE
	execute(opcode, info);
	if(state == CpuState::Faulted) return StepResult::Fault;

	cycles += info.cycles;
	if(state == CpuState::Halted) return StepResult::Halt;
	return StepResult::Ok;
}

StepResult CPU::run(unsigned long maxSteps){
	for(unsigned long i = 0; i < maxSteps; ++i){
		StepResult result = step();
		if(result != StepResult::Ok) return result;
	}
	return StepResult::Ok;				//limit reached, CPU is still Running
}

Byte CPU::fetchByte(){
	Byte value = bus.read(pc);
	pc = (Word)(pc + 1);
	return value;
}

Word CPU::fetchWord(){
	Byte low = fetchByte();
	Byte high = fetchByte();
	return makeWord(low, high);
}

void CPU::execute(Byte opcode, const OpcodeInfo& info){
	//1. read the operand bytes; their layout depends only on the format
	Byte a = 0, b = 0;					//a = first register, b = second register or immediate
	Word addr = 0;

	switch(info.format){
		case Format::Implied:
			break;
		case Format::Reg:
		case Format::RegInd:
			a = fetchByte();
			break;
		case Format::RegReg:{
			Byte both = fetchByte();
			a = (Byte)(both >> 4);		//destination in the high 4 bits
			b = (Byte)(both & 0x0F);	//source in the low 4 bits
			break;
		}
		case Format::RegImm:
			a = fetchByte();
			b = fetchByte();
			break;
		case Format::Abs:
			addr = fetchWord();
			break;
		case Format::RegAbs:
			a = fetchByte();
			addr = fetchWord();
			break;
	}
	if(busFailed()) return;

	//2. check register numbers before anything is changed
	if(info.format != Format::Implied && info.format != Format::Abs){
		if(!checkReg(a)) return;
	}
	if(info.format == Format::RegReg){
		if(!checkReg(b)) return;
	}

	//3. do the work
	switch(opcode){
		//system
		case OP_NOP:
			break;
		case OP_HLT:
			state = CpuState::Halted;
			break;

		//data movement
		case OP_MOV:
			reg[a] = reg[b];
			break;
		case OP_LDI:
			reg[a] = b;
			setZN(reg[a]);
			break;
		case OP_LD:{
			Byte value = bus.read(addr);
			if(busFailed()) return;
			reg[a] = value;
			setZN(value);
			break;
		}
		case OP_ST:
			bus.write(addr, reg[a]);
			busFailed();
			break;
		case OP_LDP:{
			Byte value = bus.read(getP());
			if(busFailed()) return;
			reg[a] = value;
			setZN(value);
			break;
		}
		case OP_STP:
			bus.write(getP(), reg[a]);
			busFailed();
			break;
		case OP_INP:
		case OP_DEP:{
			Word p = (Word)(opcode == OP_INP ? getP() + 1 : getP() - 1);
			reg[6] = highByte(p);
			reg[7] = lowByte(p);
			break;
		}

		//arithmetic and logic, register-register
		case OP_ADD:  reg[a] = add(reg[a], reg[b], false);              break;
		case OP_ADC:  reg[a] = add(reg[a], reg[b], getFlag(FLAG_C));    break;
		case OP_SUB:  reg[a] = sub(reg[a], reg[b], false);              break;
		case OP_SBC:  reg[a] = sub(reg[a], reg[b], getFlag(FLAG_C));    break;
		case OP_CMP:  sub(reg[a], reg[b], false);                       break;		//flags only
		case OP_AND:
		case OP_OR:
		case OP_XOR:
			if(opcode == OP_AND)     reg[a] = (Byte)(reg[a] & reg[b]);
			else if(opcode == OP_OR) reg[a] = (Byte)(reg[a] | reg[b]);
			else                     reg[a] = (Byte)(reg[a] ^ reg[b]);
			setZN(reg[a]);
			setFlag(FLAG_C, false);
			setFlag(FLAG_V, false);
			break;

		//arithmetic, register-immediate
		case OP_ADDI: reg[a] = add(reg[a], b, false); break;
		case OP_SUBI: reg[a] = sub(reg[a], b, false); break;
		case OP_CMPI: sub(reg[a], b, false);          break;		//flags only

		//single register
		case OP_INC:								//C is left unchanged
			reg[a] = (Byte)(reg[a] + 1);
			setZN(reg[a]);
			setFlag(FLAG_V, reg[a] == 0x80);		//127 + 1 overflowed
			break;
		case OP_DEC:								//C is left unchanged
			reg[a] = (Byte)(reg[a] - 1);
			setZN(reg[a]);
			setFlag(FLAG_V, reg[a] == 0x7F);		//-128 - 1 overflowed
			break;
		case OP_NOT:
			reg[a] = (Byte)~reg[a];
			setZN(reg[a]);
			break;
		case OP_SHL:
			setFlag(FLAG_C, getBit(reg[a], 7));
			reg[a] = (Byte)(reg[a] << 1);
			setZN(reg[a]);
			break;
		case OP_SHR:
			setFlag(FLAG_C, getBit(reg[a], 0));
			reg[a] = (Byte)(reg[a] >> 1);
			setZN(reg[a]);
			break;

		//control flow (the target is only checked when the jump is really taken)
		case OP_JMP:
			if(checkJump(addr)) pc = addr;
			break;
		case OP_JZ:
		case OP_JNZ:
		case OP_JC:
		case OP_JNC:
		case OP_JN:{
			bool take = false;
			if(opcode == OP_JZ)       take = getFlag(FLAG_Z);
			else if(opcode == OP_JNZ) take = !getFlag(FLAG_Z);
			else if(opcode == OP_JC)  take = getFlag(FLAG_C);
			else if(opcode == OP_JNC) take = !getFlag(FLAG_C);
			else                      take = getFlag(FLAG_N);
			if(take && checkJump(addr)) pc = addr;
			break;
		}
		case OP_CALL:
			if(!checkJump(addr)) return;
			//pc already points at the next instruction: that is the return address
			if(!push(highByte(pc))) return;
			if(!push(lowByte(pc))) return;
			pc = addr;
			break;
		case OP_RET:{
			Byte low = 0, high = 0;
			if(!pop(low)) return;
			if(!pop(high)) return;
			Word target = makeWord(low, high);
			if(checkJump(target)) pc = target;
			break;
		}

		//stack
		case OP_PUSH:
			push(reg[a]);
			break;
		case OP_POP:{
			Byte value = 0;
			if(pop(value)) reg[a] = value;
			break;
		}
		case OP_PUSHF:
			push(flags);
			break;
		case OP_POPF:{
			Byte value = 0;
			if(pop(value)) flags = (Byte)(value & 0x0F);		//only bits 0-3 are flags
			break;
		}

		default:
			break;		//never reached: step() only calls execute() for valid opcodes
	}
}

void CPU::setFlag(Flag flag, bool on){
	flags = on ? setBit(flags, flag) : clearBit(flags, flag);
}

void CPU::setZN(Byte result){
	setFlag(FLAG_Z, result == 0);
	setFlag(FLAG_N, getBit(result, 7));
}

//shared by ADD, ADC, ADDI: C = true sum did not fit in 8 bits,
//V = both inputs had the same sign but the result has the other sign
Byte CPU::add(Byte a, Byte b, bool carryIn){
	unsigned int sum = (unsigned int)a + b + (carryIn ? 1 : 0);
	Byte result = (Byte)(sum & 0xFF);
	setZN(result);
	setFlag(FLAG_C, sum > 0xFF);
	setFlag(FLAG_V, ((~(a ^ b)) & (a ^ result) & 0x80) != 0);
	return result;
}

//shared by SUB, SBC, SUBI, CMP, CMPI: C = 1 means a borrow happened (a was smaller),
//V = inputs had different signs and the result's sign differs from a
Byte CPU::sub(Byte a, Byte b, bool borrowIn){
	int diff = (int)a - (int)b - (borrowIn ? 1 : 0);
	Byte result = (Byte)(diff & 0xFF);
	setZN(result);
	setFlag(FLAG_C, diff < 0);
	setFlag(FLAG_V, ((a ^ b) & (a ^ result) & 0x80) != 0);
	return result;
}

//stack grows DOWN; SP points at the next free slot
bool CPU::push(Byte value){
	if(sp < MemoryMap::STACK_START){
		raise(FaultType::StackOverflow, sp, "Stack is full");
		return false;
	}
	bus.write(sp, value);
	if(busFailed()) return false;
	sp = (Word)(sp - 1);
	return true;
}

bool CPU::pop(Byte& value){
	if(sp == MemoryMap::STACK_END){
		raise(FaultType::StackUnderflow, sp, "Stack is empty");
		return false;
	}
	sp = (Word)(sp + 1);
	value = bus.read(sp);
	return !busFailed();
}

bool CPU::checkReg(int index){
	if(index < 0 || index >= NUM_REGS){
		char text[48];
		snprintf(text, sizeof(text), "Register R%d does not exist (use R0-R7)", index);
		raise(FaultType::InvalidRegister, 0, text);
		return false;
	}
	return true;
}

//ROM starts at 0x0000, so only the upper end needs checking
bool CPU::checkJump(Word target){
	if(target > MemoryMap::ROM_END){
		raise(FaultType::InvalidJumpTarget, target, "Jump target " + hex16(target) + " is outside ROM");
		return false;
	}
	return true;
}

//copies a Bus fault into the CPU (adding the PC the Bus does not know)
bool CPU::busFailed(){
	if(!bus.hasFault()) return false;
	Fault busFault = bus.getFault();
	bus.clearFault();
	raise(busFault.type, busFault.address, busFault.message);
	return true;
}

//PC is moved back to the failing instruction, so the debugger shows the line that caused it
void CPU::raise(FaultType type, Word address, const std::string& message){
	fault.type = type;
	fault.pc = instrStart;
	fault.address = address;
	fault.message = message;
	state = CpuState::Faulted;
	pc = instrStart;
}

Byte CPU::getReg(int index) const{
	if(index < 0 || index >= NUM_REGS) return 0;
	return reg[index];
}

void CPU::setReg(int index, Byte value){
	if(index < 0 || index >= NUM_REGS) return;
	reg[index] = value;
}

Word CPU::getPC() const{
	return pc;
}

Word CPU::getSP() const{
	return sp;
}

Word CPU::getP() const{
	return makeWord(reg[7], reg[6]);		//R6 = high byte, R7 = low byte
}

Byte CPU::getFlags() const{
	return flags;
}

bool CPU::getFlag(Flag flag) const{
	return getBit(flags, flag);
}

CpuState CPU::getState() const{
	return state;
}

const Fault& CPU::getFault() const{
	return fault;
}

unsigned long CPU::getCycles() const{
	return cycles;
}
