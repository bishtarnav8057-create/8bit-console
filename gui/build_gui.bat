@echo off
rem Builds the PIXEL-8 GUI and runs its tests. Run this from the "Qt 6.x.x (MinGW 64-bit)" start-menu
rem terminal (the one that has cmake, mingw32-make and Qt's 64-bit g++ on the PATH), not a plain terminal:
rem there the old 32-bit C:\MinGW g++ would be found first and the build fails.
setlocal
cd /d "%~dp0"

cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release || goto FAIL
cmake --build build || goto FAIL
ctest --test-dir build --output-on-failure || goto FAIL

rem copy the Qt DLLs next to the exe so it also runs by double-click and on a PC without Qt
where windeployqt >nul 2>nul
if %errorlevel%==0 windeployqt --release --no-translations build\pixel8_gui.exe

echo.
echo Build OK: gui\build\pixel8_gui.exe
echo Tests OK: test_games, test_sprites, test_snake_segments, test_asm_notes
exit /b 0

:FAIL
echo.
echo BUILD OR TESTS FAILED. Is this the Qt MinGW terminal, and is the Qt kit MinGW 64-bit?
exit /b 1
