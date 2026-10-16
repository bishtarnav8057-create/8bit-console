//DEVICE IS THE COMMON SHAPE "BUS" REFERS TO

#ifndef DEVICE_H
#define DEVICE_H

#include  "Bits.h"

class Device{
	public:
		virtual ~Device() {}

		virtual Byte read(Word offset) = 0;
		virtual bool write(Word offset, Byte value) = 0;   //false = refused
		virtual unsigned int size() const = 0;
		virtual const char* name() const = 0;

		//read without side effects, for debuggers and undo. Plain memory just reads;
		//IoPorts overrides it so looking at the RNG port does not use up a random number.
		virtual Byte peek(Word offset) { return read(offset); }
};

#endif
