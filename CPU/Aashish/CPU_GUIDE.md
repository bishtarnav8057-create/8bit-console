# PIXEL-8 CPU: Demo, Internals, Viva Preparation and Integration Guide

**Module:** CPU, memory, Bus, I/O ports, Console (`CPU/Aashish/src`)
**Team NULL Pointers · DSCPP-III-2026-T193**

This guide has five parts:

1. [The live demo](#1-the-live-demo-one-addition-step-by-step): what it shows and how to read the screen
2. [How the CPU module works](#2-how-the-cpu-module-works): the parts, and what the code does on every instruction
3. [Viva questions with answers](#3-viva-questions-with-answers)
4. [C/C++ concepts used](#4-cc-concepts-used-in-the-module), with a [syllabus map](#41-syllabus-map-data-structures-and-oop) of where each topic is in the code
5. [How other units connect](#5-how-the-other-units-connect-to-the-cpu): assembler, PPU, input, State Manager, games

Everything here is taken from the code in this repository. File and function names are given so any
claim can be checked by opening the file.

---

## 1. The live demo: one addition, step by step

### 1.1 Running it

```
cd CPU/Aashish
mingw32-make add              # 5 + 3
mingw32-make add A=25 B=17    # any two numbers from 0 to 255
```

| Key | Action |
|---|---|
| Enter / Space / → | next step |
| B / ← / Backspace | one step back |
| R | restart |
| N | type two new numbers |
| Q / Esc | quit |

Use Windows Terminal or PowerShell, at least 80 columns wide. The presenter script (what to say on each
screen) is in [`demo/ADD_DEMO.md`](demo/ADD_DEMO.md).

### 1.2 The idea

A CPU only ever repeats three steps:

| Phase | Plain words | What happens in PIXEL-8 |
|---|---|---|
| **FETCH** | get it | The bytes at the address in the Program Counter travel from memory over the bus into the CPU |
| **DECODE** | understand it | The first byte (the *opcode*) is looked up in the instruction table: which instruction, how many bytes, which registers |
| **EXECUTE** | do it | Registers, the ALU or memory do the work. The Program Counter now points at the next instruction |

The demo runs a 5-instruction program and shows each instruction as these three phases. That makes
15 steps in total, and each one lights up the parts of the machine the data passes through.

### 1.3 The program

| # | Assembly | Meaning | Bytes in ROM | Address |
|---|---|---|---|---|
| 1 | `LDI R0, 5` | put 5 in register R0 | `11 00 05` | 0000 |
| 2 | `LDI R1, 3` | put 3 in register R1 | `11 01 03` | 0003 |
| 3 | `ADD R0, R1` | R0 = R0 + R1 | `30 01` | 0006 |
| 4 | `ST R0, [0x8000]` | save R0 in RAM at 8000 | `13 00 00 80` | 0008 |
| 5 | `HLT` | stop | `01` | 000C |

**How to read the bytes.** The bytes are written in **hexadecimal**, not binary. One hex digit is 4 bits, so
two digits are one byte (`11` hex = 0001 0001 binary = 17 decimal). For example:

- `11 00 05`: `11` = LDI (opcode), `00` = register R0, `05` = the number 5.
- `30 01`: `30` = ADD. The byte `01` is split into two halves: high half `0` = R0 (destination),
  low half `1` = R1 (source).
- `13 00 00 80`: `13` = ST, `00` = R0, then the address `8000` is stored **low byte first**
  (`00`, then `80`). This order is called *little-endian*.
- The addresses go up unevenly (0000, 0003, 0006, 0008, 000C) because instructions are 1–4 bytes long.
  `000C` is 12 in decimal.

### 1.4 What is on the screen

| Box | Part | Role |
|---|---|---|
| MEMORY | **ROM** (0000–7FFF) | holds the program. The CPU can read it but never write it |
| MEMORY | **RAM** (8000–DFFF) | working memory. The answer is written at 8000 |
| middle | **Bus** | the connection between CPU and memory. It is drawn only while bytes are on it |
| CPU | **Program Counter (PC)** | address of the next instruction |
| CPU | **Instruction Register** | the bytes just fetched |
| CPU | **Decoder** | turns the opcode into an action, using the opcode table |
| CPU | **Registers R0, R1** | the CPU's fast storage. Shown in decimal |
| CPU | **ALU** | Arithmetic Logic Unit: the calculator |

The path the numbers take through the machine:
`ROM → bus → Instruction Register → Decoder → Registers → ALU → Registers → bus → RAM`

### 1.5 What is real and what is drawn

The class `CPU` does fetch, decode and execute inside a single call, `cpu.step()`. The demo splits that
call into three screens like this:

| Phase | How the demo gets the values |
|---|---|
| FETCH | reads the same bytes the CPU will read: `bus.peek(pc)`, `bus.peek(pc+1)`, … |
| DECODE | uses the same table the CPU uses: `OpcodeTable::lookup(opcode)` (mnemonic, length) |
| EXECUTE | really calls `cpu.step()`, then reads back registers, PC, RAM and flags |

The **Instruction Register** box is a teaching device. The CPU class keeps the fetched bytes in local
variables inside `execute()`, not in a named member. The **result screen** reads the answer from RAM 8000
and compares it with plain C++ `(Byte)(a + b)`.

Going back a step replays a fresh machine up to that point (`buildView` in `demo/add.cpp`). This is
simple and always correct, because the program is tiny and the CPU is deterministic.

---

## 2. How the CPU module works

### 2.1 The parts and who owns what

```
                         Console  (owns everything, wires it like MemoryMap.h)
   ┌──────────────────────────────────────────────────────────────────────────┐
   │                                                                          │
   │   CPU  ──── Bus& ────▶  Bus  ── finds the device for an address ──┐      │
   │   (registers, PC, SP,            (vector of Mapping{start,end,     │      │
   │    flags, state, cycles)          Device*})                        ▼      │
   │                                                                          │
   │        Rom      Ram "RAM"     Ram "VRAM"     IoPorts      Ram "STACK"    │
   │      0000-7FFF  8000-DFFF     E000-EFFF     F000-F0FF     F100-FFFF      │
   │        └──────────┴───────────────┴─────────────┴─────────────┘          │
   │                   all are a Device (read / write / peek / size / name)   │
   └──────────────────────────────────────────────────────────────────────────┘
```

| File | Role |
|---|---|
| `Bits.h/.cpp` | `Byte` (8-bit) and `Word` (16-bit) types; `getBit`, `setBit`, `clearBit`, `lowByte`, `highByte`, `makeWord` |
| `Device.h` | the abstract interface every memory chip and port implements |
| `Rom`, `Ram` | memory chips. ROM refuses every write; one `Ram` class is used for RAM, VRAM and the stack |
| `IoPorts` | buttons, random numbers, PPU control, frame counter, text output |
| `Bus` | routes each address to the right device; records faults; notifies a write listener |
| `MemoryMap.h` | every address range and port number, in one place |
| `Opcodes.h/.cpp` | the instruction table: 38 instructions, their format, length and cycle cost |
| `CPU.h/.cpp` | registers and the fetch–decode–execute loop |
| `Fault.h/.cpp` | the 7 kinds of error a program can cause |
| `Disassembler` | turns bytes back into text (`LDI R0, 0x05`) for debuggers |
| `Console` | the whole machine in one object, and the plug-in point for other units |

### 2.2 The memory map (64 KB, 16-bit addresses)

| Range | Size | Device | Notes |
|---|---|---|---|
| `0000–7FFF` | 32 KB | ROM | the program. Writes cause `IllegalWrite` |
| `8000–DFFF` | 24 KB | RAM | variables |
| `E000–EFFF` | 4 KB | VRAM | screen: 32 × 16 pixels, 1 byte each; pixel (x, y) is at `E000 + y*32 + x` |
| `F000–F0FF` | 256 B | IoPorts | `F000` buttons, `F001` random, `F002` PPU control, `F003` frame counter, `F004` text out |
| `F100–FFFF` | 3.75 KB | Stack | grows **down** from `FFFF` |

A 16-bit address can name 2¹⁶ = 65,536 bytes. That is why the map ends at `FFFF`.

### 2.3 The Bus: an address goes in, the right chip answers

`Bus::read(address)`:

1. `find(address)` finds the mapping whose `[start, end]` contains the address, by **binary search**.
   `attach()` inserts every mapping in order of `start`, and ranges never overlap, so `find` looks at the
   middle range and throws away the half the address can't be in (`Bus.cpp`, `Bus::find`).
2. If none is found, it raises `UnmappedAccess` and returns `0xFF`.
3. Otherwise it calls `device->read(address - start)`. The device only sees an **offset**, so a `Ram`
   doesn't need to know where it lives in the map.

`Bus::write` works the same way. If the device refuses (returns `false`, as ROM always does), it
raises `IllegalWrite`. If a **write listener** is set, it is called with `(address, oldValue, newValue)`
after every successful write. This is how undo and debuggers see memory changes.

`attach()` refuses a null device, a backwards range, a range larger than the chip, or a range that
**overlaps** an existing one. So two chips can never answer the same address.

`peek()` and `poke()` are the "debugger" versions. They never raise a fault and never call the listener.
`peek` also avoids side effects: reading the RNG port through `read` uses up a random number, while
`peek` only looks at it.

### 2.4 The CPU's state

| Member | Size | Meaning |
|---|---|---|
| `reg[0..7]` | 8 × 8 bits | general registers R0–R7. **R6:R7 together form P**, a 16-bit pointer (`getP()`) used by `LDP`/`STP` |
| `pc` | 16 bits | Program Counter: next instruction |
| `sp` | 16 bits | Stack Pointer: next free stack slot (starts at `FFFF`) |
| `flags` | 4 bits | Z (zero), C (carry), N (negative), V (overflow), bits 0–3 |
| `state` | enum | `Running`, `Halted` or `Faulted` |
| `cycles` | counter | total cost so far. Used for frame timing |
| `instrStart` | 16 bits | where the current instruction began, so a fault can point at it |

### 2.5 One instruction: `CPU::step()`

Simplified from `CPU.cpp` (fault checks shortened):

```cpp
StepResult CPU::step(){
	if(state == Halted)  return StepResult::Halt;
	if(state == Faulted) return StepResult::Fault;
	bus.clearFault();
	instrStart = pc;

	Byte opcode = fetchByte();                                  //FETCH: bus.read(pc), pc++
	const OpcodeInfo& info = OpcodeTable::lookup(opcode);       //DECODE: table lookup
	if(!info.valid) → InvalidOpcode fault
	execute(opcode, info);                                      //EXECUTE
	cycles += info.cycles;
	...
}
```

`execute()` works in three stages:

1. **Read the operands.** How many bytes to read, and what they mean, depends **only on the format**:

   | Format | Bytes | Example | Layout |
   |---|---|---|---|
   | Implied | 1 | `HLT`, `RET` | opcode |
   | Reg | 2 | `INC R1` | opcode, reg |
   | RegReg | 2 | `ADD R0, R1` | opcode, (dst << 4 \| src) |
   | RegImm | 3 | `LDI R0, 5` | opcode, reg, value |
   | Abs | 3 | `JMP 0x0010` | opcode, addrLow, addrHigh |
   | RegAbs | 4 | `ST R0, [0x8000]` | opcode, reg, addrLow, addrHigh |
   | RegInd | 2 | `LDP R0` | opcode, reg (address comes from P) |

2. **Check the registers.** A register number above 7 raises `InvalidRegister` before anything is changed.
3. **Do the work.** A `switch(opcode)` with one `case` per instruction.

### 2.6 Tracing 5 + 3 through the code

| Instruction | What the code does | Cycles |
|---|---|---|
| `LDI R0, 5` | fetch `11`; lookup gives RegImm, 3 bytes; read `a=0`, `b=5`; `reg[0] = 5`; `setZN(5)`: Z=0, N=0 | 3 |
| `LDI R1, 3` | same, `reg[1] = 3` | 3 |
| `ADD R0, R1` | fetch `30`; RegReg; read `01` → `a = 01>>4 = 0`, `b = 01&0x0F = 1`; `reg[0] = add(5, 3, false)` | 2 |
| `ST R0, [8000]` | fetch `13`; RegAbs; `a = 0`; `fetchWord()` = `makeWord(00, 80)` = `8000`; `bus.write(8000, 8)`, which the Bus sends to RAM at offset 0 | 5 |
| `HLT` | fetch `01`; Implied; `state = Halted` | 1 |
| | **total** (shown on the demo's result screen) | **14** |

The cycle costs come from the table in `Opcodes.cpp`: cycles = bytes fetched + extra memory accesses.
`ST` is 4 bytes + 1 write = 5.

### 2.7 How the ALU sets the flags

```cpp
Byte CPU::add(Byte a, Byte b, bool carryIn){
	unsigned int sum = (unsigned int)a + b + (carryIn ? 1 : 0);   //done in a wider type
	Byte result = (Byte)(sum & 0xFF);                             //keep the low 8 bits
	setZN(result);                                                //Z: result == 0,  N: bit 7
	setFlag(FLAG_C, sum > 0xFF);                                  //unsigned overflow
	setFlag(FLAG_V, ((~(a ^ b)) & (a ^ result) & 0x80) != 0);    //signed overflow
	return result;
}
```

| Sum | Result | Z | C | N | V | Why |
|---|---|---|---|---|---|---|
| 5 + 3 | 8 | 0 | 0 | 0 | 0 | fits |
| 250 + 10 | 4 | 0 | 1 | 0 | 0 | 260 > 255: **carry**. As signed numbers, −6 + 10 = 4 is correct, so V = 0 |
| 127 + 1 | 128 | 0 | 0 | 1 | 1 | fits unsigned, but as signed numbers +127 + 1 became −128: **overflow** |
| 255 + 1 | 0 | 1 | 1 | 0 | 0 | wraps to 0 |

**C** is for unsigned numbers (0–255), and **V** is for signed numbers (−128 to +127). The V formula
means: *both inputs had the same sign bit, and the result's sign bit is different.*

### 2.8 The stack, CALL and RET

This is an **array-based stack** (LIFO). The array is the `Ram` chip at `F100–FFFF` (a `std::vector<Byte>`),
and `SP` is the index of the top. The code is `CPU::push` and `CPU::pop` in `CPU.cpp`.

- The stack grows **down**. `SP` points at the **next free** slot and starts at `FFFF` (empty).
- `push`: write at `SP`, then `SP--`. If `SP < F100`, raise `StackOverflow`.
- `pop`: if `SP == FFFF`, raise `StackUnderflow`. Otherwise `SP++`, then read.
- `CALL addr`: the PC already points after the CALL, which is the return address. The CPU pushes its high
  byte, then its low byte, and jumps.
- `RET`: pop low, pop high, jump back. The target is checked to be inside ROM.
- Because the last address pushed is the first one popped, nested calls return in the right order: if A calls
  B and B calls C, C returns to B before B returns to A. That is exactly why a stack is used here.

### 2.9 Faults: bad programs stop cleanly

| Fault | Caused by |
|---|---|
| `InvalidOpcode` | a byte that is not in the table (e.g. `FF`) |
| `InvalidRegister` | register number above 7 |
| `IllegalWrite` | writing to ROM, or to a read-only port |
| `UnmappedAccess` | an address no device answers |
| `StackOverflow` / `StackUnderflow` | too many pushes / pop on an empty stack |
| `InvalidJumpTarget` | `JMP`/`CALL`/`RET` to an address outside ROM |

On a fault the CPU stores `type`, `pc`, `address` and `message`, sets `state = Faulted`, and moves the
**PC back to the start of the failing instruction**, so a debugger shows the line that caused it. The Bus
keeps only the **first** fault until it is cleared, because later errors are usually side effects of the
first one.

### 2.10 Speed and complexity

| Operation | Cost | Why |
|---|---|---|
| Opcode lookup | O(1) | a 256-slot array, one slot per possible byte (invalid ones are marked) |
| Bus address lookup | O(log d), d = 5 devices | binary search over the mappings, sorted by start address |
| Mnemonic lookup (`OpcodeTable::find`) | O(1) on average | `std::unordered_map` (hash map) from name to opcode |
| Recording an instruction in the trace | O(1) | enqueue into a fixed-size circular queue, no memory allocated |
| One `step()` | O(1) | a fixed amount of work per instruction |

---

## 3. Viva questions with answers

### A. Basics

1. **What is a CPU?**
   The part of a computer that runs instructions. It repeatedly fetches an instruction from memory,
   decodes it and executes it.

2. **What are fetch, decode and execute?**
   Fetch: read the instruction's bytes from the address in the PC. Decode: find out which instruction it
   is and what its operands are. Execute: do it. In our code these are `fetchByte()`, `OpcodeTable::lookup()`
   and `execute()` inside `CPU::step()`.

3. **What is a register? Why not keep everything in RAM?**
   A small storage location inside the CPU. Our CPU has eight 8-bit registers. Arithmetic works only on
   registers (`ADD R0, R1`), so values are loaded from memory, worked on, then stored back.

4. **What is the Program Counter?**
   A 16-bit register holding the address of the next instruction. Fetching moves it forward. Jumps,
   `CALL` and `RET` overwrite it.

5. **What is the ALU?**
   The Arithmetic Logic Unit. In our code it is the `add()` and `sub()` helpers plus the AND/OR/XOR/NOT/shift
   cases in `execute()`. They compute the result and set the flags.

6. **What is a bus?**
   The connection between CPU and devices. Ours (`Bus` class) takes an address and passes the read or write to
   whichever device owns that address range.

7. **Difference between ROM and RAM?**
   ROM holds the program and is read-only: `Rom::write` always returns `false`, and the Bus turns that into an
   `IllegalWrite` fault. RAM can be read and written.

8. **Why is it called 8-bit if addresses are 16-bit?**
   "8-bit" refers to the data size: registers and the ALU work on 8-bit values (0–255). Addresses are 16-bit
   so the machine can address 64 KB instead of only 256 bytes.

9. **Why hexadecimal?**
   One hex digit is exactly 4 bits, so a byte is always two digits (`00`–`FF`) and an address is always four
   (`0000`–`FFFF`). Binary would be 8 or 16 digits long.

10. **What is little-endian?**
    A 16-bit value is stored low byte first. Address `8000` is stored as `00 80`. See `makeWord(low, high)` in `Bits.cpp`.

### B. About the demo

11. **Is the demo real or just an animation?**
    Real. Fetch and decode show the same bytes and table the CPU uses (`bus.peek`, `OpcodeTable::lookup`),
    and execute calls the real `cpu.step()`. All values shown are read back from the emulator, and the
    answer is checked against C++ `a + b`.

12. **Your CPU class has no instruction register. Why does the demo show one?**
    In `CPU::execute()` the fetched operand bytes live in local variables (`a`, `b`, `addr`). Real hardware
    needs a physical register to hold them between clock steps; software doesn't. The demo draws it to show
    where those bytes are during decode.

13. **Why are instructions different lengths?**
    Each format needs different operands. `HLT` needs nothing (1 byte); `ST` needs a register and a 16-bit
    address (4 bytes). Variable length saves ROM space. The length comes from `OpcodeTable::lengthOf(format)`,
    so it can never disagree with the format.

14. **How is `ADD R0, R1` only 2 bytes when it has two registers?**
    Each register number fits in 4 bits (0–7 needs only 3), so both are packed into one byte: destination in
    the high half, source in the low half. Decoded with `both >> 4` and `both & 0x0F`.

15. **What happens with 250 + 10?**
    The true sum, 260, doesn't fit in a byte. The ALU keeps the low 8 bits (260 − 256 = 4) and sets the Carry
    flag. The demo explains this on screen.

16. **Why 14 cycles?**
    LDI 3 + LDI 3 + ADD 2 + ST 5 + HLT 1. The cost of each instruction is bytes fetched plus extra memory accesses.

### C. Design decisions

17. **Why is `Device` an abstract class?**
    So the Bus can treat ROM, RAM, VRAM, the stack and the I/O ports the same way through a `Device*`. A new
    chip only has to implement `read`, `write`, `size` and `name`. The Bus does not change.

18. **What stops two devices from claiming the same address?**
    `Bus::attach` checks every existing range for overlap (`start <= m.end && end >= m.start`) and refuses.

19. **Why does a bad program raise a fault instead of crashing or throwing an exception?**
    A broken user program is expected, not exceptional, so it is reported as data. The emulator keeps running,
    the debugger can show the fault type, the failing PC and a message, and tests can check for exact faults.

20. **Why does the Bus keep only the first fault?**
    The first error is the real cause. Anything after it is usually a side effect (`Bus::raise` returns early
    if a fault is already stored).

21. **Why move the PC back to the failing instruction?**
    So a debugger highlights the line that caused the error, not the one after it (`CPU::raise` sets `pc = instrStart`).

22. **Why a 256-entry table for opcodes?**
    An opcode is one byte, so there are exactly 256 possible values. Indexing an array is O(1), and every unused
    slot is marked invalid, so unknown bytes are caught immediately.

23. **Why does the stack grow downward?**
    It starts at the top of memory (`FFFF`), so the free space between the stack and the other regions is not
    split up. Many real CPUs do this. Our stack has its own fixed region (`F100–FFFF`), and overflow is detected.

24. **What is P (R6:R7)?**
    Two 8-bit registers joined into a 16-bit address, so `LDP`/`STP` can read or write memory at an address
    computed at run time, e.g. to walk through an array or draw pixels in a loop.

25. **What is the difference between `read` and `peek`?**
    `read` can have side effects (reading the RNG port advances the generator) and can raise faults. `peek`
    never changes anything and never faults. Debuggers, undo and the demo use `peek`.

26. **How do the random numbers work?**
    A 16-bit Galois LFSR (`IoPorts::stepRandom`: shift right, XOR with `0xB400` when the bit shifted out was 1).
    It has a fixed default seed, so a program behaves the same every run. That is needed for tests and undo.

27. **Can a program modify itself or run code from RAM?**
    No. ROM can't be written, and `checkJump` refuses any jump target above `7FFF`, so code runs only from ROM.

28. **What stops an infinite loop from freezing the emulator?**
    `CPU::run(maxSteps)` stops after a set number of instructions, and `Console::runFrame` stops after a cycle
    budget (`CYCLES_PER_FRAME = 20000`) and always ends the frame.

### D. C++ questions

29. **Why does `Device` have a virtual destructor?**
    So deleting a derived object through a `Device*` runs the derived destructor. Without it, that is
    undefined behaviour.

30. **What does `= 0` mean in `virtual Byte read(Word offset) = 0;`?**
    It is a pure virtual function. `Device` can't be created directly, and every derived class must implement it.

31. **Why does `CPU` hold a `Bus&` instead of a `Bus*` or its own `Bus`?**
    A reference can't be null and can't be reseated, so every CPU is always connected to exactly one bus. The
    bus is owned by `Console` and shared with the other units.

32. **Why does the order of members in `Console` matter?**
    Members are built in the order they are declared, not the order in the initializer list. The CPU needs the
    Bus, and the Bus needs the chips, so the chips are declared first and the CPU last (see the comment in
    `Console.h`).

33. **`enum class` vs plain `enum`?**
    `enum class` (e.g. `FaultType`, `CpuState`) is scoped and doesn't convert to int by accident. `Opcode` is a
    plain enum with `: Byte` as its underlying type, because opcodes are meant to be used as byte values in
    program arrays.

34. **Why `uint8_t`/`uint16_t` instead of `char`/`int`?**
    They have an exact, guaranteed size and are unsigned. Unsigned arithmetic wraps modulo 2⁸ or 2¹⁶, which is
    exactly how 8-bit hardware behaves. The sign of plain `char` is implementation-defined.

35. **How does undo work in C++ terms?**
    `Bus::setWriteListener` takes a `std::function`. The caller passes a lambda that records
    `(address, old, new)`. Undo writes the old values back with `poke` (newest first) and restores a CPU snapshot.

36. **Why is the opcode table built inside a function (`table()`) and not as a global?**
    A function-local `static` is created the first time it is used. A global table could be read by another
    file's global initializer before it exists (the "static initialization order" problem).

### E. Testing

37. **How did you test the CPU?**
    `tests/test_cpu.cpp` has 193 checks: bit helpers, RAM and ROM, the Bus and its binary search, every
    instruction, flags, the stack, every fault type, the I/O ports, the instruction trace queue, `operator<<`
    and `CpuSnapshot`. All pass (`mingw32-make test`).

38. **How do you know the arithmetic is correct for all inputs?**
    The calculator tests (`CPU/tests/test_calc.cpp`) run all 65,536 input pairs for + − & | ^ on our CPU and
    compare each with C++. With only 2⁸ × 2⁸ pairs, testing every case is cheap.

### F. Integration

39. **How does a `.asm` file get into the CPU?**
    Parser → operand checker → `Assembler::assembleFile()` → `AssembleResult::bytes` →
    `Console::loadProgram(bytes)` → ROM at `0000`. The calculator (`mingw32-make calc` in `CPU/`) runs this
    whole chain.

40. **How do the assembler and the CPU agree on opcodes?**
    Both use the same `Opcodes.h` and `OpcodeTable`. The assembler calls `OpcodeTable::find("ADD")` and
    `lookup(...).length`. Nothing is copied by hand.

41. **How would you add a new instruction?**
    Add the opcode to the enum in `Opcodes.h`, add one row to `ENTRIES` in `Opcodes.cpp`, and add a `case` in
    `CPU::execute()`. If it uses an existing format, the assembler, operand checker and disassembler support it
    automatically, because they all read the same table.

42. **How would you add a new device, e.g. sound?**
    Either give it a free port in `IoPorts` (offsets `05`–`FF` are unused), or write a class implementing
    `Device` and `attach()` it to a free address range. The current map is fully allocated, so a range would
    have to be taken from RAM or VRAM in `MemoryMap.h`.

### G. Data structures and the syllabus

43. **Where is a stack used in your code?**
    In three places. (1) The CPU's own stack: `CPU::push` / `CPU::pop` in `CPU.cpp`. It's an array-based stack:
    the array is the stack `Ram` chip, `SP` is the top, overflow and underflow are checked. (2) `CALL` / `RET`
    push and pop the return address, which is what makes nested calls return in the right order (LIFO).
    (3) The live demo's step-back keeps a history of steps and undoes the newest first (`hist.push_back`,
    `hist.back()`, `hist.pop_back()` in `demo/live.cpp`), and inside one step the memory writes are undone
    newest first too.

44. **Where is a queue used, and why a circular one?**
    `InstructionTrace` (`InstructionTrace.h/.cpp`) is a circular queue in a fixed array of 16 entries. The CPU
    enqueues every instruction it starts, so after a fault you can see how the program got there, oldest
    first. Circular, because `front` and `rear` wrap around the end of the array: no shifting, O(1) enqueue and
    dequeue. When it is full, the oldest entry is dequeued first, so it always holds the *last* 16 instructions
    and never grows, even after millions of steps. `std::queue` can't do this: it has no fixed capacity and
    can't be read from the middle.

45. **Why binary search in the Bus if there are only 5 devices?**
    The ranges are sorted and never overlap, which is exactly the situation binary search is made for, and
    `find` runs on *every* memory access. With 5 devices the speed difference is small (at most 3 checks
    instead of 5). The real gain is that the lookup stays O(log n) if more devices are added. The cost is paid
    once, in `attach()`, which keeps the list sorted when it inserts.

46. **Where is a hash map used?**
    `OpcodeTable::find` (mnemonic → opcode) uses a `std::unordered_map<std::string, Byte>`. The assembler and the
    operand checker call it for every line of a program. The name is turned to upper case and hashed straight
    to its bucket, O(1) on average, instead of comparing it against all 38 names. The other direction
    (opcode → info) needs no hashing at all: the opcode byte itself is the index into a 256-slot array (a
    direct-address table, the simplest kind of hash table).

47. **Where are friend functions and friend classes used, and why not just getters/setters?**
    *Friend function:* `operator<<(std::ostream&, const CPU&)`, so `std::cout << cpu` prints a register dump. It
    can't be a member function, because the stream, not the CPU, is on the left of `<<`. Being a friend lets it
    read the private registers directly.
    *Friend class:* `CpuSnapshot` saves and restores *everything* inside the CPU, including the fault with all
    its details and the instruction trace. The public setters can't put a fault back (`setState` only keeps or
    clears one). Adding a public setter for each private member would let any code change them. `friend`
    gives that access to one trusted class only, so encapsulation stays intact.

48. **Why is there no linked list?**
    Because nothing in the CPU fits one. Every list here is either fixed-size (registers, the opcode table, the
    trace) or read by position (memory, the sorted device list that binary search needs). Linked lists can't be
    indexed or binary-searched, they use extra memory for the pointers, and they scatter nodes around memory. A
    linked list is the right choice when items are often inserted or removed in the middle, and the CPU never
    does that. The one place in the project where it fits is undo/redo history in the State Manager: add and
    drop at both ends in O(1), with a cursor moving back and forth.

---

## 4. C/C++ concepts used in the module

| Concept | Where | Why it is used |
|---|---|---|
| **Fixed-width types** (`uint8_t`, `uint16_t`) with `typedef` | `Bits.h`: `Byte`, `Word` | exact 8- and 16-bit sizes, unsigned wrap-around like real hardware |
| **Bitwise operators** `& \| ^ ~ << >>` | `Bits.cpp`, `CPU::add/sub`, RegReg decoding, LFSR | flags, packing two registers in one byte, splitting words into bytes |
| **Casting and modular arithmetic** | `(Byte)(sum & 0xFF)`, `(Word)(pc + 1)` | explicit 8/16-bit wrap-around |
| **Classes and encapsulation** | `CPU`, `Bus`, `Rom`, `Ram`, `IoPorts`, `Console` | state is `private`; changed only through methods such as `setReg`, `setPC` |
| **Abstract base class, pure virtual functions** | `Device` | one interface for every chip |
| **Inheritance and `override`** | `Rom`, `Ram`, `IoPorts : public Device` | each chip implements the interface; `override` makes the compiler check the signature |
| **Runtime polymorphism** | `Bus` stores `Device*` and calls `device->read()` | the Bus doesn't need to know which chip it is talking to |
| **Virtual destructor** | `virtual ~Device() {}` | safe deletion through a base pointer |
| **Default virtual implementation** | `Device::peek` calls `read`; `IoPorts` overrides it | only devices with side effects need a special peek |
| **References as members** | `CPU` holds `Bus&`; `Disassembler` holds `Bus&` | always connected, never null |
| **Constructor initializer lists, member order** | `Console::Console()`, `CPU::CPU(Bus&)` | members built in a safe order |
| **`const` member functions and `const&` returns** | `getReg() const`, `const Fault& getFault() const` | read-only access with no copies |
| **Reference out-parameters** | `bool pop(Byte& value)` | return success *and* a value |
| **`enum class`** | `FaultType`, `CpuState`, `StepResult`, `Format` | scoped, type-safe states |
| **Enum with an underlying type** | `enum Opcode : Byte` | opcodes are byte values |
| **`struct` aggregates** | `OpcodeInfo`, `Fault`, `Mapping`, `SystemState` | plain records of data |
| **Static members** | `OpcodeTable::lookup()`; `Console::CYCLES_PER_FRAME` with an out-of-class definition in `Console.cpp` | table access without an object; a class constant that can be used by address |
| **Function-local static** | `table()` in `Opcodes.cpp` | built on first use; avoids initialization-order problems |
| **Anonymous namespace** | `hex8`, `hex16` in `CPU.cpp`, `Bus.cpp`; `ENTRIES` in `Opcodes.cpp` | helpers visible only inside one file (internal linkage) |
| **Function overloading** | `Console::loadProgram(const Byte*, unsigned)` and `loadProgram(const std::vector<Byte>&)` | accepts raw arrays and vectors |
| **Default arguments** | `runFrame(unsigned long cycleBudget = CYCLES_PER_FRAME)`, `reset(Word start = 0x0000)` | common case needs no arguments |
| **Operator overloading** | `operator<<` for `CPU`; `operator==` / `!=` for `SystemState` (`CPU/src/SystemState.h`) | `std::cout << cpu`; compare snapshots in tests |
| **Friend function** | `operator<<(std::ostream&, const CPU&)`, declared `friend` in `CPU.h` | a non-member (the stream is on the left) that reads private registers |
| **Friend class** | `friend class CpuSnapshot;` in `CPU.h` | one trusted class saves and restores every private member, the fault included |
| **Constructor overloading, `explicit`** | `CpuSnapshot()` and `explicit CpuSnapshot(const CPU&)` | an empty save point, or one taken from a CPU; `explicit` stops a CPU turning into a snapshot by accident |
| **STL containers** | `std::vector` (chips' bytes, Bus mappings), `std::unordered_map` (mnemonic → opcode), `std::string`, `std::array` (snapshots) | automatic memory management, bounds known |
| **STL algorithms** | `std::fill`, `std::copy` in `Rom::load`, `Ram::clear` | clear and load memory |
| **`std::function` and lambdas** | `Bus::setWriteListener`, `IoPorts::setPpuListener`, `setConsoleListener` | other units plug in their own code without the CPU knowing about them (callbacks / observer pattern) |
| **Header guards** | `#ifndef CPU_H ...` in every header | each header included once |
| **Preprocessor conditionals** | `#ifdef _WIN32` in the demos | Windows console code kept out of the portable core |
| **`nullptr`, `override`, `enum class`, lambdas** | throughout | C++11 features (the project builds with `-std=c++11`) |
| **Error handling by return value** | `bool write(...)`, `StepResult`, `Fault` | expected errors are data, not exceptions |

### 4.1 Syllabus map: data structures and OOP

**Data structures**

| Topic | Where in the code | What it does there |
|---|---|---|
| **Array** | `Byte reg[8]` (`CPU.h`); 256-slot `OpcodeInfo slot[256]` and `ENTRIES[]` (`Opcodes.cpp`); `items[16]` (`InstructionTrace.h`) | register file; O(1) opcode decode by index; storage for the circular queue |
| **Stack** (array-based, LIFO) | `CPU::push` / `CPU::pop`, `CALL` / `RET`, `PUSHF` / `POPF` (`CPU.cpp`); the step-back history in `demo/live.cpp` | saving registers and return addresses so nested calls come back in the right order; undo newest first |
| **Queue** (circular) | `InstructionTrace` (`InstructionTrace.h/.cpp`), filled in `CPU::step` | keeps the last 16 instructions, oldest first, so a fault shows how the program got there |
| **Binary search** | `Bus::find` (`Bus.cpp`); `Bus::attach` keeps the list sorted by inserting in order | finds the chip for an address in O(log n) |
| **Hash map** | `std::unordered_map` in `OpcodeTable::find` (`Opcodes.cpp`); the 256-slot opcode table is a direct-address table | mnemonic → opcode for the assembler in O(1) on average |
| **Linked list** | *not used, on purpose* | nothing in the CPU inserts or removes in the middle; see viva question 48 |

The assembler (Arnav) also uses `std::map` (a balanced binary search tree) for label → address and
address → source line.

**C++ / OOP**

| Topic | Where in the code |
|---|---|
| **Class and object** | `CPU`, `Bus`, `Ram`, `Rom`, `IoPorts`, `Console`, `Disassembler`, `InstructionTrace`, `CpuSnapshot`; `Console` builds one object of each chip |
| **Encapsulation** | every class keeps its data `private` (`reg`, `pc`, `sp`, `mappings`, ...) and exposes methods |
| **Abstraction** | `Device` is an abstract class (pure virtual `read`, `write`, `size`, `name`); the Bus only knows that interface |
| **Inheritance** | `Ram`, `Rom`, `IoPorts : public Device` |
| **Polymorphism** | run time: the Bus calls `device->read()` through a `Device*`, and `IoPorts` overrides `peek`. Compile time: `loadProgram` is overloaded, the two `CpuSnapshot` constructors, `operator<<` |
| **Constructors** | `explicit CPU(Bus&)`, `Ram(size, name)`, `Console()` with an initializer list in a set order, the default constructors of `Fault`, `InstructionTrace` and `CpuSnapshot`; virtual destructor in `Device` |
| **Friend function** | `operator<<` for `CPU` |
| **Friend class** | `CpuSnapshot` for `CPU` |
| **Static members** | `OpcodeTable::lookup/find/lengthOf`, `CPU::NUM_REGS`, `InstructionTrace::CAPACITY`, `Console::CYCLES_PER_FRAME` |
| **Functions** | default arguments (`reset(Word start = 0x0000)`), pass by reference (`pop(Byte&)`), `const` member functions |

---

## 5. How the other units connect to the CPU

### 5.1 The full system

```
  game.asm ──▶ Parser ──▶ OperandCheck ──▶ Assembler ──bytes──▶ Console::loadProgram()      (done)
                                                                      │
                     ┌────────────────────────────────────────────────┘
                     ▼
   keyboard ──▶ io().setButton() ──▶ [F000]                                                    (hook ready)
                                       │
                                CPU ⇄ Bus ⇄ ROM / RAM / VRAM / IoPorts / Stack                 (done)
                                       │                 │
   State Manager ◀── bus().setWriteListener()      VRAM [E000…] ──▶ pixel(x, y) ──▶ PPU ──▶ screen
   (undo/rewind)  ── bus().poke() + restoreState()                                             (PPU not built)
```

All units connect through **one object, `Console`** (`Console.h`), and never by reaching into the CPU's
private members. The table below lists the hooks each unit uses:

| Unit (owner) | Uses | Status |
|---|---|---|
| Parser, Operand checker, Assembler (Arnav) | `OpcodeTable::find/lookup`, `Format`, `MemoryMap.h` → `loadProgram(bytes)` | **done and connected**: the calculator runs `.asm` programs on the CPU |
| Emulation controller / game loop (Arnav) | `step()`, `runFrame()`, `cpu().getState()`, `getFault()` | hooks ready |
| PPU / graphics (Ashmit) | `pixel(x, y)` or `vram()`, `io().getPpuControl()`, `io().setPpuListener(...)` | **PPU not built yet**; the CPU can already write pixels to VRAM |
| Input (Ashmit) | `io().setButton(IoPorts::BTN_LEFT, true)` or `io().setInput(bits)` | hook ready |
| State Manager / undo (Piyush) | `bus().setWriteListener(...)`, `bus().poke(...)`, `captureState` / `restoreState` | `SystemState` done; undo is proven in the live dashboard's step-back |
| Debugger | `Disassembler::line()`, `bus().peek()`, `AssembleResult::lineAt` | demos use these |
| Games (Pixel Dodge, Snake) | must be rewritten in PIXEL-8 assembly, drawing into VRAM | **not yet**: today they are separate C++ programs with their own screen array |

### 5.2 What a game loop will look like

This sketch uses the real APIs. `readKeyboard()` and `drawPixel()` are placeholders for the
input and PPU code that hasn't been written yet.

```cpp
Console console;
AssembleResult program = Assembler().assembleFile("pixel_dodge.asm");
if(!program.ok){ /* show program.errors, with line numbers */ }
console.loadProgram(program.bytes);

while(playing){
	console.io().setInput(readKeyboard());                 //INPUT: one bit per button into port F000
	StepResult r = console.runFrame();                     //CPU: one frame of game logic, then FRAME port +1
	for(int y = 0; y < MemoryMap::SCREEN_HEIGHT; ++y)      //PPU: read VRAM, draw it
		for(int x = 0; x < MemoryMap::SCREEN_WIDTH; ++x)
			drawPixel(x, y, console.pixel(x, y));
	if(r == StepResult::Fault) showError(console.cpu().getFault());
}
```

Inside the game program, the CPU waits for the frame port (`F003`) to change. That way the game
logic runs once per frame, whatever the cycle budget is.

### 5.3 How undo (State Manager) plugs in

```cpp
struct Change { Word address; Byte oldValue; };
std::vector<Change> changes;
console.bus().setWriteListener([&](Word address, Byte oldValue, Byte){
	changes.push_back({address, oldValue});            //every memory write is recorded
});

SystemState before = captureState(console.cpu());      //registers, flags, PC, SP, state, cycles
console.step();

//undo that step
for(auto it = changes.rbegin(); it != changes.rend(); ++it)
	console.bus().poke(it->address, it->oldValue);      //memory back, newest first, no listener, no fault
restoreState(console.cpu(), before);                    //CPU back
```

The CPU didn't need any change for this. The listener and `poke` were designed in from the start.

### 5.4 Why the design makes future units easy

- **One shared definition of the instruction set** (`Opcodes.h`/`.cpp`). The CPU, assembler, operand checker
  and disassembler can't disagree, and a new instruction is added in one place.
- **One memory map** (`MemoryMap.h`). The PPU knows VRAM starts at `E000` and the input code knows the
  button port is `F000`, from the same constants the CPU uses.
- **Devices behind an interface.** New hardware is a new `Device` subclass attached to the Bus. The CPU
  doesn't change.
- **Callbacks instead of dependencies.** The CPU core has no knowledge of the PPU, the State Manager or the
  screen. They register `std::function` listeners, so each unit can be built and tested on its own.
- **Deterministic.** The fixed RNG seed and cycle-counted frames mean the same input always gives the same
  output, which tests, replays and undo rely on.

### 5.5 What is still to do (honest status)

1. Build the PPU: read VRAM through `pixel(x, y)` and draw it every frame.
2. Map real keys to `io().setButton(...)`.
3. Rewrite Pixel Dodge in PIXEL-8 assembly so it runs on our CPU (today it is a separate C++ program
   with a 40 × 22 screen, while our VRAM is 32 × 16).
4. A step debugger that loads any `.asm` file. The live dashboard's stepper and undo already cover most of this.

See `PROJECT_RETROSPECTIVE.md` (repository root) for the full status.
