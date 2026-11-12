# PIXEL-8 GUI (Calculator + Arcade)

One window with two tabs:

| Tab | What it does |
|---|---|
| **Calculator** | Pick two numbers (0-255) and an operation (+ - * / AND OR XOR). The answer is worked out by **our CPU running `CPU/programs/calc.asm`**. The window shows the answer (decimal, hex, binary), carry / borrow / divide-by-zero warnings, the CPU flags, and a table of the `calc.asm` lines that ran with the register values. |
| **Arcade** | Snake and Pong, same rules as `pixel/pixel8_arcade.cpp`, in a proper window. |

Nothing in `CPU/` or `pixel/` is changed. The GUI only calls `Calculator` (from `CPU/calc`) and has its own copy of the game rules.

## Build (Windows)

You need **Qt 6 with the MinGW 64-bit kit** (the same one the team's `ui/` folder uses).

**Qt Creator:** File > Open File or Project > `gui/CMakeLists.txt` > kit *Desktop Qt 6.x MinGW 64-bit* > Run.

**Command line:** open the *Qt 6.x.x (MinGW 64-bit)* terminal from the Start menu and run

```
gui\build_gui.bat
gui\build\pixel8_gui.exe
```

`build_gui.bat` also runs `windeployqt`, so the exe runs by double-click afterwards.
The program finds `calc.asm` by itself (`CPU/programs/calc.asm`). If you move the exe somewhere else, use the **Choose calc.asm...** button.

## Using it

**Calculator:** type the numbers, choose the operation, press **Calculate** (or Enter). Try `200 + 100` (carry), `5 - 9` (borrow), `15 * 20` (too big), `17 / 5` (remainder 2), `7 / 0` (divide by zero).

**Arcade:** click **Snake** or **Pong**.

| Key | Action |
|---|---|
| Arrow keys / W A S D | Snake: turn. Pong: move your paddle |
| P or Space | Pause / continue |
| R | Restart |
| M or Esc | Back to the menu |

Pong: first to 5 points wins. Snake: hitting a wall or yourself ends the game.

## Files

| File | Job | Qt? |
|---|---|---|
| `src/Games.h/.cpp` | Rules of Snake and Pong (the game logic) | no |
| `src/GameBoard.h/.cpp` | Widget that draws the 30 x 20 board of squares | yes |
| `src/GameTab.h/.cpp` | Arcade tab: buttons, timer, keyboard | yes |
| `src/CalculatorTab.h/.cpp` | Calculator tab: inputs, answer, CPU trace table | yes |
| `src/MainWindow.h/.cpp`, `src/main.cpp` | The window with the two tabs, menu, About box | yes |
| `tests/test_games.cpp` | 12 checks for Snake and Pong | no |
| `CMakeLists.txt`, `build_gui.bat` | Build | |

The split is the same idea as the CPU code: the rules (`Games`, `Calculator`) know nothing about windows, so they can be tested without a window. The Qt files only show them.

## Tests

`gui\build\test_games.exe` (built together with the GUI), or with only g++:

```
g++ -std=c++11 gui/src/Games.cpp gui/tests/test_games.cpp -o gui/test_games.exe
gui\test_games.exe
```

The calculator's own checks are the existing `CPU/tests/test_calc.cpp` (`mingw32-make test` in `CPU/`).

## Notes

* Colours and the board size (`W`, `H` in `Games.h`) are easy to change.
* Game speed: `SNAKE_MS` and `PONG_MS` at the top of `GameTab.cpp`.
