#include "SystemState.h"

SystemState captureState(const CPU& cpu) {
    SystemState s;
    for (int i = 0; i < CPU::NUM_REGS; ++i) s.registers[i] = cpu.getReg(i);
    s.zeroFlag = cpu.getFlag(CPU::FLAG_Z);
    s.carryFlag = cpu.getFlag(CPU::FLAG_C);
    s.negativeFlag = cpu.getFlag(CPU::FLAG_N);
    s.overflowFlag = cpu.getFlag(CPU::FLAG_V);
    s.programCounter = cpu.getPC();
    s.stackPointer = cpu.getSP();
    s.cpuState = cpu.getState();
    s.cycles = cpu.getCycles();
    return s;
}

void restoreState(CPU& cpu, const SystemState& s) {
    for (int i = 0; i < CPU::NUM_REGS; ++i) cpu.setReg(i, s.registers[i]);
    Byte flags = 0;
    if (s.zeroFlag) flags = setBit(flags, CPU::FLAG_Z);
    if (s.carryFlag) flags = setBit(flags, CPU::FLAG_C);
    if (s.negativeFlag) flags = setBit(flags, CPU::FLAG_N);
    if (s.overflowFlag) flags = setBit(flags, CPU::FLAG_V);
    cpu.setFlags(flags);
    cpu.setPC(s.programCounter);
    cpu.setSP(s.stackPointer);
    cpu.setCycles(s.cycles);
    cpu.setState(s.cpuState);
}

bool operator==(const SystemState& a, const SystemState& b) {
    return a.registers == b.registers &&
           a.zeroFlag == b.zeroFlag && a.carryFlag == b.carryFlag &&
           a.negativeFlag == b.negativeFlag && a.overflowFlag == b.overflowFlag &&
           a.programCounter == b.programCounter && a.stackPointer == b.stackPointer &&
           a.cpuState == b.cpuState && a.cycles == b.cycles;
}

bool operator!=(const SystemState& a, const SystemState& b) { return !(a == b); }
