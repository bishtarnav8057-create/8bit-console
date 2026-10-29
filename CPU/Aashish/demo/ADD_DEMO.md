# One Addition, Step by Step: presenter script

The simplest CPU demo. A single addition, 5 + 3, travels through every part of the CPU.
It takes about 3 minutes, needs 15 presses of Enter, and assumes the audience knows nothing about CPUs.

```
mingw32-make add              # 5 + 3
mingw32-make add A=25 B=17    # any two numbers 0-255 (or press N inside the demo)
```

Window: at least 80 columns. Keys: **Enter** next · **B / ←** back · **R** restart · **N** new numbers · **Q** quit.
Each press plays a short animation that lights up the parts one after another. Pressing any key skips it.

---

## The one idea to get across

> A CPU only ever does three things, over and over: **FETCH** an instruction from memory,
> **DECODE** what it means, **EXECUTE** it. Everything a computer does is built from that loop.

## What is on the screen

| Part | Say it like this |
|---|---|
| **Memory: ROM** | "The program is stored here, as plain numbers." |
| **Memory: RAM** | "Scratch paper. The answer will be written here, at address 8000." |
| **Bus** | "The wire between memory and the CPU. Every number travels along it." |
| **Program Counter** | "A bookmark. It says which instruction comes next." |
| **Instruction Register** | "Holds the instruction the CPU just fetched." |
| **Decoder** | "The translator. It turns the number into an action." |
| **Registers R0, R1** | "Tiny, very fast boxes inside the CPU. It can only calculate with values in here." |
| **ALU** | "The calculator inside the CPU." |

## Screen by screen

| Screen | Point to | Say |
|---|---|---|
| Intro | the table | "We want 5 + 3. The CPU can't read that, so it becomes 5 tiny instructions, and those become bytes." |
| 1 FETCH | PC → ROM → bus → IR | "The bookmark says 0000. Those bytes travel along the bus into the CPU. Memory doesn't know what they mean." |
| 2 DECODE | Decoder | "11 means *load a number*. Then *which box* (R0), then *the number* (5)." |
| 3 EXECUTE | R0 | "5 goes into box R0. The bookmark moves to the next instruction." |
| 4–6 | same path | "Same three steps again: now 3 goes into R1." *(go faster here)* |
| 7–8 | | "Fetch, decode: 30 means ADD." |
| **9 EXECUTE** | R0, R1 → ALU → R0 | **"Here is the addition. Both numbers go into the ALU, it adds them, and 8 comes back into R0."** |
| 10–12 | R0 → bus → RAM | "ST = store. The 8 travels back along the bus, this time *out* of the CPU, into RAM." |
| 13–15 | | "HLT = stop." |
| Result | the journey line | "The answer was read back from memory. 5 instructions × 3 phases. A real computer does this billions of times per second." |

## Likely questions

- **"Is this animation or the real CPU?"** The real emulator. Every number on screen is read back from
  `src/` (the CPU, Bus, ROM and RAM classes). `cpu.step()` does fetch, decode and execute in one call. The
  demo shows the three phases separately using the same bytes the CPU reads (`bus.peek` at the PC) and the
  same opcode table (`OpcodeTable::lookup`), then really runs `cpu.step()` for EXECUTE. The result screen
  checks the answer in RAM against plain C++ `a + b`.
- **"What if the sum is bigger than 255?"** Try `A=250 B=10`. A register is 8 bits, so it wraps to 4 and
  the Carry flag turns on. The demo explains this on screen.
- **"Why hexadecimal?"** It is the usual way to write memory: two digits = one byte. Register values are
  shown in normal decimal.
- **Deeper questions** (stack, faults, graphics, I/O): use the full dashboard, `mingw32-make live`
  (see `LIVE_DEMO.md`).
