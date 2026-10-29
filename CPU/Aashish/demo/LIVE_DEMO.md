# PIXEL-8 Live Dashboard Demo

`demo/live.cpp` is an interactive terminal dashboard that teaches how the PIXEL-8 CPU and memory work.
It runs the **real** emulator code from `src/` (Bus, CPU, Disassembler, IoPorts, Console). Nothing on screen
is faked or pre-recorded. Every number you see comes from actually executing the instruction.

Unlike the older scrolling demo (`demo/demo.cpp`), the live demo keeps **one screen** and redraws it on every
key press. So the audience always sees the program, the registers, the flags and the memory at the same time,
plus one plain sentence that says what the last instruction did.

---

## 1. Building and running

From `CPU/Aashish/`:

```
mingw32-make live        # Windows (MinGW)
make live                # Linux
```

This compiles every file in `src/` together with `demo/live.cpp` into `live.exe` and starts it.
`mingw32-make clean` removes it again.

**Terminal requirements**

| Need | Why |
|---|---|
| At least **100 columns** wide | Every screen is laid out for 100 columns. The menu shows a yellow warning if the window is narrower. |
| ANSI colour support (Windows Terminal or PowerShell) | Colours, box lines and lamps use escape codes. On Windows the program switches on "virtual terminal" mode. If that fails it prints a message and tells you to use `mingw32-make demo` instead. |
| UTF-8 | The boxes (`┌─┐`), arrows (`▶ ▸ →`) and pixels (`██`) are UTF-8. The program sets the console code page to 65001 itself. |

The demo uses the terminal's alternate screen, so when you quit (or press Ctrl+C) your previous terminal
contents come back untouched.

---

## 2. The menu

```
 1  Memory map and Bus          where every address goes, and what gets refused
 2  Fetch → Decode → Execute    step through a loop that adds 10 + 9 + ... + 1
 3  Flags                       type any sum, watch Z C N V light up
 4  Stack, CALL and RET         a subroutine call, step by step
 5  Faults                      broken programs are caught, never crash
 6  Graphics                    the CPU draws an X into video memory
 7  I/O ports                   buttons, frames, random numbers, text
 Q  quit
```

The scenes are ordered so that each one builds on the previous: where data lives → how instructions run →
how the CPU makes decisions → how subroutines work → what happens when things go wrong → how the CPU
reaches the screen and the outside world. `Esc` always returns to this menu.

---

## 3. The stepper dashboard (scenes 2, 4, 5 and 6)

Scenes 2, 4, 5 and 6 load a small machine-code program into ROM and run it through the `Stepper` class,
one instruction per key press. The screen looks like this:

```
┌ PIXEL-8 ▸ Scene 2 · Fetch → Decode → Execute ─────────────────── step 7 · cycles 23 ┐
 Add 10 + 9 + ... + 1. Watch R1 count down, R0 grow, ...
┌─ PROGRAM in ROM ────────┐ ┌─ REGISTERS ────────────┐ ┌─ STACK ──────────────────┐
│ ✓ 0006 loop: ADD R0, R1 │ │ R0  0x13  19 ◀ was 10  │ │ empty: nothing pushed yet│
│ ▶ 0008       DEC R1     │ │ R1  0x09   9           │ │ SP → FFFF next free slot │
│   000A       JNZ loop   │ │ ...                    │ │                          │
│   000D save: ST R0, ... │ │ PC  0x0008  next instr │ │ MEMORY WATCH             │
│                         │ │ SP  0xFFFF  empty      │ │ RAM 0x8000 = 0x00 = 0    │
│                         │ │ P   0x0000  R6:R7      │ │                          │
└─────────────────────────┘ └────────────────────────┘ └──────────────────────────┘
 FLAGS   Z zero   C carry   N negative   V overflow
┌─ WHAT JUST HAPPENED (step 7) ─────────────────────────────────────────────────────┐
│ FETCH 30 01 from 0x0006 ▸ DECODE 0x30 = ADD, 2 registers, 2 bytes ▸ EXECUTE +3 cycles │
│ → R0 = R0 + R1  →  10 + 9 = 19                                                      │
│ Flags: ...                                                                          │
└───────────────────────────────────────────────────────────────────────────────────┘
```
*(sketch; exact spacing and values differ on screen)*

### What each panel shows

| Panel | Contents |
|---|---|
| **PROGRAM in ROM** | The program disassembled by the real `Disassembler`. `▶` = next instruction (PC), `✓` = the one that just ran, `✗` = the instruction that faulted. Jump targets are shown by label name (`loop`, `twice`, ...) instead of raw addresses. |
| **REGISTERS** | R0–R7 in hex and decimal. A register that just changed is highlighted yellow with `◀ was N`. Below: PC, SP (with how many stack bytes are in use) and P, the 16-bit pointer formed by R6:R7. |
| **STACK** | The used part of the stack, top at the `▸`. Each byte is labelled with *why* it is there: `return addr hi/lo` (from CALL), `saved R0` (from PUSH), `saved flags` (from PUSHF). Some scenes add a **MEMORY WATCH** on one RAM address. |
| **FLAGS** | Four lamps, Z C N V, lit green when set. |
| **WHAT JUST HAPPENED** | Line 1: the **fetch → decode → execute** strip (raw bytes fetched, opcode decoded with its operand format and length, cycles added). Line 2: a plain-English sentence with the actual numbers. Line 3: which flags changed, or an explanation of wrap-around / faults. |

Scene 6 swaps the register and stack panels for a **picture of VRAM** (the 32 × 16 screen): pixels written
by the last step are yellow, older pixels green.

When the CPU halts, a green bar summarises the result. When it faults, a red bar names the fault and the PC.

### Keys

| Key | Action |
|---|---|
| `Enter` / `Space` / `→` / `n` | Execute one instruction |
| `Backspace` / `←` / `b` | **Step back**: undo the last instruction completely |
| `R` | Run to the end, animated. Any key pauses. |
| `S` / `Home` | Start over (undo every step) |
| `Esc` / `q` | Back to the menu |

### How step back works

Stepping backwards is not a re-run from the start. Each step stores:

1. a **snapshot** of the CPU (`Snap`: registers, PC, SP, flags, cycle count) taken *before* the instruction, and
2. every **memory write** the instruction made, recorded by the Bus's *write listener*
   (`bus.setWriteListener`) as `(address, old value, new value)`.

Undo restores memory first, newest write first, using `bus.poke`, then restores the CPU snapshot and
clears any fault. This is the same mechanism the team's State Manager uses for rewind, so the demo is
also a working proof that the hook is sufficient.

---

## 4. The scenes

### Scene 1 · Memory map and Bus

**Concept:** a 16-bit address space (64 KB) is split between chips, and the Bus routes each request to
exactly one of them.

```
0x0000 ┌──────────────┐
       │ ROM    32 KB │  the program, read-only
0x8000 ├──────────────┤
       │ RAM    24 KB │  variables
0xE000 ├──────────────┤
       │ VRAM    4 KB │  screen pixels
0xF000 ├──────────────┤
       │ I/O    256 B │  ports
0xF100 ├──────────────┤
       │ STACK 3840 B │  grows down from 0xFFFF
0xFFFF └──────────────┘
```

Six requests are sent one at a time; the chip each one lands in lights up on the map:

| # | Request | Result | Lesson |
|---|---|---|---|
| 1 | `bus.write(0x8005, 0x42)` | ok | Bus subtracts the chip's start address: RAM offset 5 |
| 2 | `bus.write(0xE001, 0x07)` | ok | VRAM offset 1 = pixel (1, 0) |
| 3 | `bus.writeWord(0x8005, 0x1234)` | ok | 16-bit values are stored **little-endian**: 0x34 then 0x12 |
| 4 | `bus.write(0x0003, 0x99)` | **refused** | ROM rejects writes → `IllegalWrite` fault, reported instead of crashing |
| 5 | `bus.read(0xF000)` | **refused** | No chip attached there on this board → `UnmappedAccess` |
| 6 | `bus.attach(chip at 0x8000)` | **refused** | Two chips may never overlap, so every address has exactly one owner |

This board deliberately has no I/O chip, so request 5 can show an unmapped access. Scene 7 uses the full Console where the ports exist.

### Scene 2 · Fetch → Decode → Execute

**Concept:** the instruction cycle, loops and conditional jumps.

```
start: LDI R0, 0
       LDI R1, 10
loop:  ADD R0, R1
       DEC R1
       JNZ loop        ; jump back while R1 != 0
save:  ST  R0, [0x8000]
       HLT
```

Step it and watch R1 count down while R0 accumulates. The key moment is `DEC R1` setting **Z** when R1
reaches 0, which makes `JNZ` fall through instead of jumping back. The explanation line says this explicitly
("Z is ON, so no jump, carry on with the next line"). The memory watch shows 0x8000 receiving the answer, **55**.

### Scene 3 · Flags

**Concept:** what Z, C, N and V mean, and the difference between unsigned and signed overflow.

Type any `A + B` or `A - B` (values −128 to 255). The demo builds a 4-instruction program
(`LDI R0, A ; LDI R1, B ; ADD/SUB R0, R1 ; HLT`), runs it on a fresh CPU, and shows the result as unsigned,
signed, hex and binary, with a reason next to every lamp.

Built-in examples (keys `a`–`e`):

| Key | Sum | What it shows |
|---|---|---|
| `a` | 255 + 1 | Result wraps to 0: **Z** and **C** on (unsigned overflow) |
| `b` | 127 + 1 | 128 = −128 signed: **N** and **V** on, C off (signed overflow only) |
| `c` | 5 − 5 | **Z** on |
| `d` | 0 − 1 | Borrow: **C** on, result 255 = −1, **N** on |
| `e` | −128 − 1 | Signed overflow downwards: **V** on |

Examples `a` and `b` together are the best way to show that **C is for unsigned and V is for signed**.

### Scene 4 · Stack, CALL and RET

**Concept:** how a subroutine call remembers where to come back to.

```
main:  LDI R0, 5
       CALL twice      ; push return address 0x0006, jump
back:  LDI R2, 1
       HLT
twice: PUSH R0
       POP  R1
       ADD  R0, R1     ; R0 = 2 × R0
       RET             ; pop 0x0006 back into PC
```

Points to show in the STACK panel:
- `CALL` pushes **two bytes** (labelled `return addr hi` / `return addr lo`) and SP moves **down** by 2.
- `PUSH R0` adds a byte labelled `saved R0`; `POP R1` removes it.
- `RET` pops the return address into PC. At the end SP is back at 0xFFFF, and the stack is empty again.

Result: R0 = 10, R2 = 1 (proof that execution came back to `main`).

### Scene 5 · Faults

**Concept:** a broken program must never crash the emulator. The CPU stops itself, names the error, and
leaves PC pointing at the bad instruction.

Pick one of seven tiny broken programs and step it on the dashboard:

| # | Program | Fault |
|---|---|---|
| 1 | Byte `0xFF` (not an opcode) | `InvalidOpcode` |
| 2 | `LDI R9, 1` (only R0–R7 exist) | `InvalidRegister` |
| 3 | `ST R0, [0x0010]` (write to ROM) | `IllegalWrite` |
| 4 | `LD R0, [0xF000]` (no chip there) | `UnmappedAccess` |
| 5 | `POP R0` on an empty stack | `StackUnderflow` |
| 6 | `JMP 0x9000` (outside the program) | `InvalidJumpTarget` |
| 7 | `PUSH R0` in an endless loop | `StackOverflow` |

Program 7 is meant to be run with `R`. The first 20 steps animate, then it runs at full speed, because it
needs 3840 pushes to fill the stack. Step back works after a fault too, so you can rewind to just before
the error.

### Scene 6 · Graphics

**Concept:** the CPU has no "draw" instruction. Graphics is just writing bytes into VRAM, and the
PPU later turns those bytes into pixels.

The program draws a full top row, a full bottom row, and two diagonals (an X) on the 32 × 16 screen,
using the pointer P = R6:R7 with `STP` (store at P) and `INP` (P + 1). The diagonals move P by 33
(down-right) or 31 (down-left) with `ADDI R7, n` followed by `ADC R6, R5`. This is a good moment to show
**16-bit arithmetic on an 8-bit CPU**: when the low byte overflows, the carry flag is added into the
high byte.

Press `R` to watch the picture build up. The explanation line converts each VRAM address into a pixel
coordinate, e.g. `0xE021 = pixel (1, 1)`.

### Scene 7 · I/O ports

**Concept:** memory-mapped I/O. The CPU talks to hardware by reading and writing special addresses.
This scene uses the full `Console`, where the ports are attached at 0xF000.

| Panel | Port(s) | What to do |
|---|---|---|
| Buttons | `0xF000` INPUT | Arrows, `Z` (A), `X` (B), `S` (Start), `D` (Select) toggle buttons. The CPU re-runs `LD R0, [0xF000]`, and R0's bits light up one per button. |
| Text out | `0xF004` | `T` runs a program that writes `'H'`, `'I'`, `' '` to the port; the printed text appears. |
| Frames + random | `0xF003` FRAME, `0xF001` RNG | `F` runs one frame. The program waits for the frame counter to change (like a game waiting for the next screen refresh), then saves a random byte to RAM 0x8000. Earlier values are listed. |

The bottom line lists the team integration hooks: `runFrame()` (game loop), `pixel()` / `setButton()`
(PPU and input), and the write listener / `poke()` (State Manager).

---

## 5. Suggested presentation flow (about 10 minutes)

1. **Scene 1** (1 min): press Enter six times. Stress little-endian (request 3) and "refused, not crashed" (requests 4–6).
2. **Scene 2** (2–3 min): step manually through two loop iterations, explaining the FETCH/DECODE/EXECUTE strip. Then press `R` to finish. Press Backspace a few times to show rewind.
3. **Scene 3** (2 min): examples `a` then `b`: the C vs V distinction.
4. **Scene 4** (2 min): step through slowly while pointing at the STACK panel labels.
5. **Scene 5** (1 min): open #3 (ROM write) and #7 (overflow with `R`).
6. **Scene 6** (1 min): press `R` and let the X draw.
7. **Scene 7** (1 min): hold a couple of buttons, press `F` a few times.

Practise once in the exact terminal and window size you will present with.

---

## 6. Code structure (`live.cpp`)

| Section | Responsibility |
|---|---|
| Terminal helpers | Colour codes, key reading (arrow keys arrive as two codes on Windows), `box()` panels, `hjoin()` side-by-side layout, `wrap()`, and `show()`, which redraws the whole screen in one write from the top-left corner so it does not flicker. `vlen()` measures visible width, ignoring colour codes and counting a UTF-8 character as one column. |
| `Board` | ROM, RAM, VRAM and stack chips attached to a `Bus` as in `MemoryMap.h` (no I/O chip), plus the CPU, the Disassembler and the write-listener hook. |
| `Scene` | Data for one stepper scene: title, goal, program bytes, labels, screen vs stack view, watched address, run speed, and an end-of-run summary. |
| `Stepper` | Step / back / run / start-over, `explain()` (builds the strip and the plain-English sentence for each opcode), and the panel renderers. |
| `sceneMemory`, `sceneFlags`, `sceneFaults`, `sceneIo` | The non-stepper scenes. |
| `loopScene`, `callScene`, `drawScene` | Programs for scenes 2, 4 and 6, written as raw bytes with assembly in the comments. |

**Adding a new stepper scene:** write a function returning a `Scene` (program bytes and labels), add a menu
entry, and call `Stepper(sc).run()`. The explanation text comes automatically from `explain()`.

**Piped input:** if stdin is not a console, keys are read from it line by line and animations are skipped.
That lets the demo be driven by a script (e.g. `printf '2\n\n\nq\nq\n' | ./live.exe`) for smoke testing.

---

## 7. Limitations

- Layout is fixed at 100 columns. Narrower windows wrap and break the panels.
- The `explain()` sentences cover every opcode used in the scenes. Any opcode without a case falls back to just the mnemonic.
- `R` redraws every step only for the first `slowSteps` steps, then every 64 steps. So on very long runs the screen updates in jumps.
- Built and tested for Windows (MinGW). The POSIX branch compiles, but there is no raw-mode keyboard input on Linux, so keys need Enter after them, arrow keys do not work, and `R` runs without animation.
