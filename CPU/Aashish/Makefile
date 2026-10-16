# Builds and runs the CPU + memory tests.
# Windows (MinGW):  mingw32-make        Linux:  make
# Clean:            mingw32-make clean

CXX      = g++
CXXFLAGS = -std=c++11 -Wall -Wextra

SRC = src/Bits.cpp src/Fault.cpp src/Ram.cpp src/Rom.cpp src/Bus.cpp \
      src/Opcodes.cpp src/CPU.cpp src/Disassembler.cpp
HDR = $(wildcard src/*.h)

TEST = test_cpu.exe

test: $(TEST)
	./$(TEST)

$(TEST): $(SRC) $(HDR) tests/test_cpu.cpp
	$(CXX) $(CXXFLAGS) $(SRC) tests/test_cpu.cpp -o $(TEST)

# "del" works in cmd/PowerShell, "rm" in Git Bash/Linux
clean:
	-del $(TEST) || rm -f $(TEST)

.PHONY: test clean
