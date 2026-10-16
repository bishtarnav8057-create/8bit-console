//THE POST OFFICE: ADDRESS IN, RIGHT CHIP OUT

#ifndef BUS_H
#define BUS_H

#include <vector>
#include <functional>
#include "Device.h"
#include "Fault.h"

typedef std::function<void(Word address, Byte oldValue, Byte newValue)> WriteListener;

class Bus{
	public:
		Bus();

		bool attach(Word start, Word end, Device* device);

		Byte read(Word address);
		bool write(Word address, Byte value);
		Word readWord(Word address);			//little-endian: low byte first
		bool writeWord(Word address, Word value);

		//for the debugger and the State Manager: never raise a fault, never call the listener
		Byte peek(Word address);				//no side effects; 0xFF if no chip is there
		bool poke(Word address, Byte value);	//undo writes old values back with this; false if refused

		bool hasFault() const;
		const Fault& getFault() const;
		void clearFault();

		void setWriteListener(WriteListener listener);

	private:
		struct Mapping{
			Word start;
			Word end;
			Device* device;
		};

		std::vector<Mapping> mappings;
		Fault fault;
		WriteListener listener;

		Mapping* find(Word address);
		void raise(FaultType type, Word address, const std::string& message);
};

#endif
