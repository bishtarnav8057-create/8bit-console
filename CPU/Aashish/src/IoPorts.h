//THE I/O PORTS: HOW A PROGRAM TALKS TO THE KEYBOARD, THE PPU AND THE TEXT CONSOLE

#ifndef IOPORTS_H
#define IOPORTS_H

#include <functional>
#include <string>
#include "Device.h"

//attached at 0xF000-0xF0FF. Offsets match MemoryMap::PORT_*:
//  0x00 INPUT        read: one bit per button (1 = held)              write: refused
//  0x01 RNG          read: next random byte                           write: new seed
//  0x02 PPU_CTRL     read/write: control byte for the PPU (bit meanings are the PPU side's)
//  0x03 FRAME        read: frame counter, +1 every frame (wait for it to change = vsync)
//  0x04 CONSOLE_OUT  write: one text character to the debug console    read: 0
//any other offset reads 0xFF and refuses writes (the Bus turns that into IllegalWrite)
class IoPorts : public Device{
	public:
		enum Button { BTN_UP = 0, BTN_DOWN = 1, BTN_LEFT = 2, BTN_RIGHT = 3,
		              BTN_A = 4, BTN_B = 5, BTN_START = 6, BTN_SELECT = 7 };		//bit positions

		IoPorts();

		Byte read(Word offset) override;			//reading RNG uses up a number
		Byte peek(Word offset) override;			//same value, but changes nothing
		bool write(Word offset, Byte value) override;
		unsigned int size() const override;
		const char* name() const override;

		void reset();								//buttons up, frame 0, default seed, console empty

		//keyboard side (Ashmit's input / Arnav's controller)
		void setButton(Button button, bool held);
		void setInput(Byte buttons);
		Byte getInput() const;

		//frame timing: the Console calls this once per frame
		void nextFrame();
		Byte getFrame() const;

		//random numbers: a fixed seed gives the same game every time (useful for tests and undo)
		void seedRandom(Word seed);
		Word getRandomState() const;

		//PPU side
		Byte getPpuControl() const;
		void setPpuListener(std::function<void(Byte value)> listener);

		//text the program printed through CONSOLE_OUT
		const std::string& getConsoleText() const;
		void clearConsoleText();
		void setConsoleListener(std::function<void(char c)> listener);

	private:
		Byte input;
		Byte frame;
		Word rng;
		Byte ppuControl;
		std::string consoleText;
		std::function<void(Byte)> ppuListener;
		std::function<void(char)> consoleListener;

		void stepRandom();
};

#endif
