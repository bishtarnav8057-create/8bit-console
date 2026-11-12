@echo off
rem Builds the PIXEL-8 GUI. Run this from the "Qt 6.x.x (MinGW 64-bit)" start-menu terminal
rem (the one that has cmake, mingw32-make and g++ on the PATH).
setlocal
cd /d "%~dp0"

cmake -S . -B build -G "MinGW Makefiles" || goto FAIL
cmake --build build || goto FAIL

rem copy the Qt DLLs next to the exe so it also runs by double-click
where windeployqt >nul 2>nul
if %errorlevel%==0 windeployqt build\pixel8_gui.exe

echo.
echo Build OK  ->  gui\build\pixel8_gui.exe
echo Tests     ->  gui\build\test_games.exe
exit /b 0

:FAIL
echo.
echo BUILD FAILED. Is this the Qt MinGW terminal, and is the Qt kit MinGW 64-bit?
exit /b 1
