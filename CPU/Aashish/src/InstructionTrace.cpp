#include "InstructionTrace.h"

const int InstructionTrace::CAPACITY;		//the value is in InstructionTrace.h; this gives it an address

InstructionTrace::InstructionTrace(){
	clear();
}

void InstructionTrace::enqueue(Word pc, Byte opcode){
	if(isFull()){
		TraceEntry oldest;
		dequeue(oldest);
	}
	int rear = (front + count) % CAPACITY;
	items[rear].pc = pc;
	items[rear].opcode = opcode;
	count++;
}

bool InstructionTrace::dequeue(TraceEntry& entry){
	if(isEmpty()) return false;
	entry = items[front];
	front = (front + 1) % CAPACITY;
	count--;
	return true;
}

void InstructionTrace::clear(){
	for(int i = 0; i < CAPACITY; ++i){
		items[i].pc = 0;
		items[i].opcode = 0;
	}
	front = 0;
	count = 0;
}

bool InstructionTrace::isEmpty() const{
	return count == 0;
}

bool InstructionTrace::isFull() const{
	return count == CAPACITY;
}

int InstructionTrace::size() const{
	return count;
}

TraceEntry InstructionTrace::at(int i) const{
	if(i < 0 || i >= count){
		TraceEntry none = {0, 0};
		return none;
	}
	return items[(front + i) % CAPACITY];
}
