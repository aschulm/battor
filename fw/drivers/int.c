#include <avr/io.h>
#include <avr/interrupt.h>

#include "int.h"

void int_enable(uint8_t level)
{
	PMIC.CTRL |= level; 
}

void int_disable(uint8_t level)
{
	PMIC.CTRL &= ~level;
}
