#ifndef SYSTEMSTATE_H
#define SYSTEMSTATE_H

#include <array>
#include <cstdint>

// A small state container for an 8-bit CPU. Adapt names/widths to the final ISA.
struct SystemState {
    // The existing Fault.h mentions register numbers 0..7, so use eight byte registers.
    std::array<std::uint8_t, 8> registers{};
    bool zeroFlag = false;
    bool carryFlag = false;
    bool negativeFlag = false;
    bool interruptFlag = false;
    std::uint16_t programCounter = 0;
    std::uint16_t stackPointer = 0xFFFF;
};

#endif
