#include "common.h"

#include "usbpower.h"

void usbpower_init()
{
	PORTE.DIR = SPI_SS_PIN_bm; // put the pin in output mode
	gpio_on(&PORTE, SPI_SS_PIN_bm);
}

void usbpower_set(uint8_t on)
{
	if (on)
	{
		gpio_on(&PORTE, SPI_SS_PIN_bm);	
	}
	else
	{
		gpio_off(&PORTE, SPI_SS_PIN_bm);	
	}
}
