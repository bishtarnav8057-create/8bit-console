#ifndef SYSTEMSTATE_H
#define SYSTEMSTATE_H

#include <array>
#include "../Aashish/src/CPU.h"
#include "../Aashish/src/MemoryMap.h"

// A snapshot of the CPU, matching CPU/Aashish/src/CPU.h exactly:
// R0-R7 (R6:R7 is the pointer P), flags Z C N V, 16-bit PC and SP.
// Fill it with captureState(cpu) instead of copying fields by hand.
struct SystemState {
    std::array<Byte, CPU::NUM_REGS> registers{};
    bool zeroFlag = false;
    bool carryFlag = false;
    bool negativeFlag = false;
    bool overflowFlag = false;
    Word programCounter = 0x0000;
    Word stackPointer = MemoryMap::STACK_END;   // empty stack, same as CPU::reset()
    CpuState cpuState = CpuState::Running;
    unsigned long cycles = 0;
};

SystemState captureState(const CPU& cpu);
// Puts a snapshot back into the CPU (registers, flags, PC, SP, state, cycles).
// Memory is not part of the snapshot: undo it with Bus::poke using the write listener's old values.
void restoreState(CPU& cpu, const SystemState& state);
bool operator==(const SystemState& a, const SystemState& b);
bool operator!=(const SystemState& a, const SystemState& b);

#endif

