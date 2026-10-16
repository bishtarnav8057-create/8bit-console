//READ-ONLY CHIP THAT HOLDS THE PROGRAM

#ifndef ROM_H
#define ROM_H

#include <vector>
#include "Device.h"

class Rom : public Device{
	public:
		explicit Rom(unsigned int size);

		Byte read(Word offset) override;
		bool write(Word offset, Byte value) override;		//always refuses
		unsigned int size() const override;
		const char* name() const override;

		bool load(const Byte* program, unsigned int length);	//only way to fill ROM

	private:
		std::vector<Byte> data;
};

#endif
