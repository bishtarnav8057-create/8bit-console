//READ/WRITE MEMORY CHIP (USED FOR BOTH RAM AND VRAM)

#ifndef RAM_H
#define RAM_H

#include <vector>
#include "Device.h"

class Ram : public Device{
	public:
		Ram(unsigned int size, const char* name);

		Byte read(Word offset) override;
		bool write(Word offset, Byte value) override;
		unsigned int size() const override;
		const char* name() const override;

		void clear();		//set every byte to 0

	private:
		std::vector<Byte> data;
		const char* label;
};

#endif
