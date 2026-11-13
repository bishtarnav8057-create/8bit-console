# PIXEL-8 GUI: a window with the Calculator and the Arcade (Snake + Pong).
#
# Qt Creator:  File > Open File or Project... > pick this file > "Desktop Qt 6.x MinGW 64-bit" kit > Run
# Command line (Qt MinGW terminal, from the repo root):  build_gui.bat    (or see gui/README.md)
#
# The CPU sources are compiled in place from ../CPU (nothing in CPU/ is changed).

cmake_minimum_required(VERSION 3.16)
project(Pixel8Gui VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

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

set(GUI_SOURCES
    src/main.cpp
    src/MainWindow.h   src/MainWindow.cpp
    src/CalculatorTab.h src/CalculatorTab.cpp
    src/screens/GameScreen.h src/screens/GameScreen.cpp
    src/widgets/OverlayMenu.h src/widgets/OverlayMenu.cpp
    src/GameBoard.h    src/GameBoard.cpp
    src/Games.h        src/Games.cpp
    src/Theme.h        src/Theme.cpp
    src/art/Sprites.h  src/art/Sprites.cpp
    src/art/SpriteCache.h src/art/SpriteCache.cpp
    src/model/SnakeSegments.h src/model/SnakeSegments.cpp
    src/widgets/Pixel.h src/widgets/Pixel.cpp
    src/widgets/PixelDialog.h src/widgets/PixelDialog.cpp
    src/screens/Screen.h
    src/screens/HomeScreen.h src/screens/HomeScreen.cpp
    src/screens/HelpScreen.h src/screens/HelpScreen.cpp
    src/screens/ConsoleScreen.h src/screens/ConsoleScreen.cpp
    src/widgets/CpuViews.h src/widgets/CpuViews.cpp
    src/Programs.h src/Programs.cpp
)

add_executable(pixel8_gui WIN32 ${GUI_SOURCES} ${CPU_SOURCES} resources.qrc)
target_include_directories(pixel8_gui PRIVATE ${CPU_DIR} src)
target_compile_definitions(pixel8_gui PRIVATE PIXEL8_CPU_DIR="${CPU_DIR}" APP_VERSION="${PROJECT_VERSION}" PIXEL8_GUI_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(pixel8_gui PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# Qt-free tests (game rules, sprite grids, snake segments):  ctest
add_executable(test_games tests/test_games.cpp src/Games.cpp)
add_executable(test_sprites tests/test_sprites.cpp src/art/Sprites.cpp)
add_executable(test_snake_segments tests/test_snake_segments.cpp src/model/SnakeSegments.cpp src/Games.cpp)
target_include_directories(test_sprites PRIVATE src)
target_include_directories(test_snake_segments PRIVATE src)
enable_testing()
add_test(NAME games COMMAND test_games)
add_test(NAME sprites COMMAND test_sprites)
add_test(NAME snake_segments COMMAND test_snake_segments)
