# PIXEL-8 GUI: a window with the Calculator and the Arcade (Snake + Pong).
#
# Qt Creator:  File > Open File or Project... > pick this file > "Desktop Qt 6.x MinGW 64-bit" kit > Run
# Command line (Qt MinGW terminal, from the repo root):  build_gui.bat    (or see gui/README.md)
#
# The CPU sources are compiled in place from ../CPU (nothing in CPU/ is changed).

cmake_minimum_required(VERSION 3.16)
project(Pixel8Gui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

get_filename_component(CPU_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../CPU" ABSOLUTE)

# same lists as CORE, TOOLS and CALC in CPU/Makefile
set(CPU_SOURCES
    ${CPU_DIR}/Aashish/src/Bits.cpp
    ${CPU_DIR}/Aashish/src/Fault.cpp
    ${CPU_DIR}/Aashish/src/Ram.cpp
    ${CPU_DIR}/Aashish/src/Rom.cpp
    ${CPU_DIR}/Aashish/src/Bus.cpp
    ${CPU_DIR}/Aashish/src/Opcodes.cpp
    ${CPU_DIR}/Aashish/src/CPU.cpp
    ${CPU_DIR}/Aashish/src/Disassembler.cpp
    ${CPU_DIR}/Aashish/src/IoPorts.cpp
    ${CPU_DIR}/Aashish/src/Console.cpp
    ${CPU_DIR}/Aashish/src/InstructionTrace.cpp
    ${CPU_DIR}/Aashish/src/CpuSnapshot.cpp
    ${CPU_DIR}/src/Parser.cpp
    ${CPU_DIR}/src/OperandCheck.cpp
    ${CPU_DIR}/src/Assembler.cpp
    ${CPU_DIR}/src/SystemState.cpp
    ${CPU_DIR}/calc/Calculator.cpp
)

# works with Qt 6 or Qt 5
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_executable(pixel8_gui WIN32
    src/main.cpp
    src/MainWindow.h   src/MainWindow.cpp
    src/CalculatorTab.h src/CalculatorTab.cpp
    src/GameTab.h      src/GameTab.cpp
    src/GameBoard.h    src/GameBoard.cpp
    src/Games.h        src/Games.cpp
    ${CPU_SOURCES}
)
target_include_directories(pixel8_gui PRIVATE ${CPU_DIR})
target_compile_definitions(pixel8_gui PRIVATE PIXEL8_CPU_DIR="${CPU_DIR}")
target_link_libraries(pixel8_gui PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# the rules of the games can be tested without Qt:  ctest   or   build/test_games
add_executable(test_games tests/test_games.cpp src/Games.cpp)
enable_testing()
add_test(NAME games COMMAND test_games)
