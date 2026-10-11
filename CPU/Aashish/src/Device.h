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
};

#endif
