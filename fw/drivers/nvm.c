#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stddef.h>
#include "nvm.h"

// taken from http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&t=87213
uint8_t nvm_read_calibration_byte(uint8_t index)
{
	uint8_t result;

	NVM.CMD = NVM_CMD_READ_CALIB_ROW_gc; // tell the NVM we will read calibration data
	result = pgm_read_byte(index);
	NVM.CMD = NVM_CMD_NO_OPERATION_gc; // clear out the NVM command register
	return result;
}
