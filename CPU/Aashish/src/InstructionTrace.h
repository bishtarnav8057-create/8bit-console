//THE FLIGHT RECORDER: THE LAST FEW INSTRUCTIONS THE CPU STARTED, OLDEST FIRST

#ifndef INSTRUCTIONTRACE_H
#define INSTRUCTIONTRACE_H

#include "Bits.h"

struct TraceEntry{
	Word pc;		//where the instruction started
	Byte opcode;
};

//a circular queue in a fixed array. front is the oldest entry, the rear is front + count,
//and both wrap around the end of the array. When it is full, enqueue drops the oldest entry
//first, so it always holds the LAST CAPACITY instructions and never grows, however long the program runs.
class InstructionTrace{
	public:
		static const int CAPACITY = 16;

		InstructionTrace();

		void enqueue(Word pc, Byte opcode);		//adds at the rear
		bool dequeue(TraceEntry& entry);		//takes the oldest off the front; false if empty
		void clear();

		bool isEmpty() const;
		bool isFull() const;
		int  size() const;
		TraceEntry at(int i) const;				//0 = oldest, size()-1 = newest; {0, 0} outside that

	private:
		TraceEntry items[CAPACITY];
		int front;
		int count;
};

#endif
