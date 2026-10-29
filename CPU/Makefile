# Builds everyone's CPU-side code together.
# Windows (MinGW):  mingw32-make <target>        Linux:  make <target>
#
#   calc              the calculator: type sums, the CPU works them out
#   calc ARGS=demo    the calculator showing a fixed list of examples
#   test              every test suite (CPU, parser, operand check, assembler, calculator)
#   clean             delete the built .exe files

CXX      = g++
CXXFLAGS = -std=c++11 -Wall -Wextra

# Aashish: memory, bus and CPU
CORE = Aashish/src/Bits.cpp Aashish/src/Fault.cpp Aashish/src/Ram.cpp Aashish/src/Rom.cpp \
       Aashish/src/Bus.cpp Aashish/src/Opcodes.cpp Aashish/src/CPU.cpp Aashish/src/Disassembler.cpp \
       Aashish/src/IoPorts.cpp Aashish/src/Console.cpp Aashish/src/InstructionTrace.cpp Aashish/src/CpuSnapshot.cpp

# Arnav and Piyush: parser, operand checker, assembler, system state
TOOLS = src/Parser.cpp src/OperandCheck.cpp src/Assembler.cpp src/SystemState.cpp

CALC = calc/Calculator.cpp

HDR = $(wildcard Aashish/src/*.h) $(wildcard src/*.h) $(wildcard calc/*.h)

TESTS = test_cpu.exe test_parser.exe test_operand_check.exe test_assembler.exe test_calc.exe

calc: calc.exe
	./calc.exe $(ARGS)

calc.exe: $(CORE) $(TOOLS) $(CALC) $(HDR) demo/calc.cpp
	$(CXX) $(CXXFLAGS) $(CORE) $(TOOLS) $(CALC) demo/calc.cpp -o $@

test: $(TESTS)
	./test_cpu.exe
	./test_parser.exe
	./test_operand_check.exe
	./test_assembler.exe
	./test_calc.exe

test_cpu.exe: $(CORE) $(HDR) Aashish/tests/test_cpu.cpp
	$(CXX) $(CXXFLAGS) $(CORE) Aashish/tests/test_cpu.cpp -o $@

test_parser.exe: $(CORE) $(TOOLS) $(HDR) tests/test_parser.cpp
	$(CXX) $(CXXFLAGS) $(CORE) $(TOOLS) tests/test_parser.cpp -o $@

test_operand_check.exe: $(CORE) $(TOOLS) $(HDR) tests/test_operand_check.cpp
	$(CXX) $(CXXFLAGS) $(CORE) $(TOOLS) tests/test_operand_check.cpp -o $@

test_assembler.exe: $(CORE) $(TOOLS) $(HDR) tests/test_assembler.cpp
	$(CXX) $(CXXFLAGS) $(CORE) $(TOOLS) tests/test_assembler.cpp -o $@

test_calc.exe: $(CORE) $(TOOLS) $(CALC) $(HDR) tests/test_calc.cpp programs/calc.asm
	$(CXX) $(CXXFLAGS) $(CORE) $(TOOLS) $(CALC) tests/test_calc.cpp -o $@

# "del" works in cmd/PowerShell, "rm" in Git Bash/Linux
clean:
	-del calc.exe $(TESTS) || rm -f calc.exe $(TESTS)

.PHONY: calc test clean
