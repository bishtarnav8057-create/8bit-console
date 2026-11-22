# PIXEL-8 GUI

The desktop window of PIXEL-8: one machine with its own interface. It starts with a boot self-test,
then a launcher with four programs. Two of them run on **our CPU** (Calculator, Console); the two games
are a C++ preview until the assembly version exists.

Nothing in `CPU/` or `pixel/` is changed. The GUI compiles the CPU sources in place from `../CPU`.

| Screen | What it does |
|---|---|
| **Boot** | A real self-test on a `Console`: RAM and VRAM keep a written byte, ROM refuses a write, `sum.asm` must halt with 55 in RAM, `calc.asm` must assemble. Any key skips. Can be turned off in Settings. |
| **Home** | Program cards (arrows or 1-4, Enter), a live preview of the focused card (`dot.asm` really running, Snake and Pong played by bots), Help, About, Settings, Exit. |
| **Calculator** | Two bytes and an operation; the answer comes from the CPU running `CPU/programs/calc.asm`. Shows the bits, CARRY / BORROW / TOO BIG / DIV BY 0 explained, the registers, flags and the lines of `calc.asm` that ran. Click a trace row for the registers at that point; REPLAY walks the rows. |
| **Console** | The emulator itself: load any `.asm`, run it frame by frame, see its 32 x 16 screen, the buttons it reads, the registers, its text output and the last 16 instructions. HLT and faults are shown; a broken file shows its errors by line. |
| **Snake / Pong** | READY, PLAYING, PAUSED (Esc), GAME OVER. Sprites, best score kept. Rules in `src/Games.cpp` (from `pixel/pixel8_arcade.cpp`). |
| **Help (F1)** | Controls for every screen, how a key reaches the CPU, the memory map to scale, the screen colours. |

## Build (Windows)

You need **Qt 6 with the MinGW 64-bit kit** (built and tested with Qt 6.12.0, MinGW 13.1, CMake 3.30).

**Qt Creator:** File > Open File or Project > `gui/CMakeLists.txt` > kit *Desktop Qt 6.x MinGW 64-bit* > Run.

**Command line:** open the *Qt 6.x.x (MinGW 64-bit)* terminal from the Start menu (not a plain terminal:
there the team's old 32-bit `C:\MinGW` g++ is found first) and run

```
gui\build_gui.bat
gui\build\pixel8_gui.exe
```

`build_gui.bat` builds a Release, runs the tests and runs `windeployqt`, so the `gui\build` folder also works
on a PC without Qt. The program finds `CPU/programs/*.asm` by itself (the path is fixed at build time, then
it looks next to the exe and up to 3 folders above). If you move the exe, use **Locate calc.asm** or
**Load .asm** in the window.

## Keys

| Where | Keys |
|---|---|
| Everywhere | **F1** help, **F11** full screen, **Esc** back (on Home: exit, always with a confirmation) |
| Home | **arrows** choose, **Enter** or **1-4** launch |
| Calculator | type **0-255** in A and B (**Tab** switches, **Up/Down** or the wheel step), **+ - \* / & \| ^** pick the operation, **Enter** runs |
| Console | **arrows / WASD** D-pad, **Z / J** = A, **X / K** = B, **Enter** = START, **Shift** = SELECT, **Space** run/pause, **F5** one frame while paused, **Ctrl+O** load a file |
| Snake | **Enter / Space** start, **arrows / WASD** turn, **P / Space / Esc** pause menu, **R** restart |
| Pong | **Enter / Space** start, **Up/Down** or **W/S** (hold) move, **P / Space / Esc** pause menu, **R** restart |

The A / B / START / SELECT keys and the colours for VRAM values 2-7 (amber, red, cyan, white, grey, dark
green; 1 and 8-255 are green) are proposals for the assembly-game author to confirm.

## Tests

```
ctest --test-dir gui\build          (or run the three test_*.exe in gui\build)
```

| Test | Checks |
|---|---|
| `test_games` | 12 checks of the Snake and Pong rules |
| `test_sprites` | every sprite grid has the right size and only palette colours, also after palette swaps |
| `test_snake_segments` | which snake tile and rotation each cell uses: straight parts, the 4 corners, head and tail |

The CPU side keeps its own tests (`mingw32-make test` in `CPU/`). `tests/programs/` holds three small
programs for the Console by hand: `bad.asm` (4 errors), `fault.asm` (a StackUnderflow at run time) and
`hello.asm` (text output and coloured pixels).

**Screenshots without a screen:** `pixel8_gui.exe --shots <folder>` saves a PNG of every screen and state
(boot, home previews, calculator sums, console states, games, help, dialogs, the minimum window size) and
quits. It uses its own settings, so it never changes your best score. With `QT_QPA_PLATFORM=offscreen` it
runs without opening a window.

## Files

| Folder | What is in it | Qt? |
|---|---|---|
| `src/Games.*`, `src/model/` | Rules of Snake and Pong; which snake tile goes where | no |
| `src/art/Sprites.*` | Every icon, tile and glyph as a text grid, and the palette | no |
| `src/art/SpriteCache.*` | Grid to crisp pixmap, whole-pixel scaling for 125 % / 150 % screens | yes |
| `src/Theme.*`, `assets/theme/theme.qss` | Colours, fonts, notched boxes, key caps, bezel, the 3 settings | yes |
| `src/widgets/` | PixelButton, Panel, KeyCap, Led, headers and footers, dialogs, overlay menu, bit rows, registers, the VRAM screen | yes |
| `src/screens/` | Boot, Home, Calculator, Console, Game (Snake and Pong), Help | yes |
| `src/MainWindow.*`, `src/main.cpp` | Switching screens, the exit/about/settings dialogs, F1/F11, start-up | yes |
| `src/Programs.*` | Finding `.asm` files and assembling one without crashing | yes |
| `assets/fonts/` | Press Start 2P and JetBrains Mono, both SIL Open Font License 1.1 (licence files next to them) | |
| `assets/icon/`, `tools/make_icon.py` | The exe icon, made from the logo sprite | |

The split is the same idea as the CPU code: the rules and the machine know nothing about windows, and no
widget gets a `Console` or `CPU` object, only plain data to show.
