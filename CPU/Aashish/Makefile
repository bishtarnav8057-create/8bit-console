# Builds and runs the CPU + memory tests.
# Windows (MinGW):  mingw32-make        Linux:  make
# Clean:            mingw32-make clean

CXX      = g++
CXXFLAGS = -std=c++11 -Wall -Wextra

SRC = src/Bits.cpp src/Fault.cpp src/Ram.cpp src/Rom.cpp src/Bus.cpp \
      src/Opcodes.cpp src/CPU.cpp src/Disassembler.cpp src/IoPorts.cpp src/Console.cpp \
      src/InstructionTrace.cpp src/CpuSnapshot.cpp
HDR = $(wildcard src/*.h)

TEST = test_cpu.exe
DEMO = demo.exe
LIVE = live.exe

test: $(TEST)
	./$(TEST)

$(TEST): $(SRC) $(HDR) tests/test_cpu.cpp
	$(CXX) $(CXXFLAGS) $(SRC) tests/test_cpu.cpp -o $(TEST)

# live demo for presentations: mingw32-make demo
demo: $(DEMO)
	./$(DEMO)

$(DEMO): $(SRC) $(HDR) demo/demo.cpp
	$(CXX) $(CXXFLAGS) $(SRC) demo/demo.cpp -o $(DEMO)

# live dashboard demo (one screen, redrawn on every key): mingw32-make live
live: $(LIVE)
	./$(LIVE)

$(LIVE): $(SRC) $(HDR) demo/live.cpp
	$(CXX) $(CXXFLAGS) $(SRC) demo/live.cpp -o $(LIVE)

# simplest demo: one addition travelling through the CPU. mingw32-make add   or   mingw32-make add A=25 B=17
ADD = add.exe

add: $(ADD)
	./$(ADD) $(A) $(B)

$(ADD): $(SRC) $(HDR) demo/add.cpp
	$(CXX) $(CXXFLAGS) $(SRC) demo/add.cpp -o $(ADD)

# "del" works in cmd/PowerShell, "rm" in Git Bash/Linux
clean:
	-del $(TEST) $(DEMO) $(LIVE) $(ADD) || rm -f $(TEST) $(DEMO) $(LIVE) $(ADD)

.PHONY: test demo live add clean
