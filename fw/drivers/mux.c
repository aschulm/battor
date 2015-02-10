#include <avr/io.h>

#include "mux.h"
#include "gpio.h"

void mux_init()
{
	PORTD.OUT &= ~MUX_IN_bm; // start with mux on NC
	PORTD.DIR |= MUX_IN_bm; // ports set to output
}

void mux_select(uint8_t dir)
{
	if (dir == MUX_R)
		gpio_on(&PORTD, MUX_IN_bm);	
	if (dir == MUX_GND)
		gpio_off(&PORTD, MUX_IN_bm);	
}
