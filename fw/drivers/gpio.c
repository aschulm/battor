#include <avr/io.h>

#include "gpio.h"

inline void gpio_on(PORT_t* port, uint8_t pin)
{
	port->OUT |= pin;
}

inline void gpio_off(PORT_t* port, uint8_t pin)
{
	port->OUT &= ~pin; 
}

inline uint8_t gpio_read(PORT_t* port, uint8_t pin_p)
{
	uint8_t in = port->IN;
	return ((in >> pin_p) & 0x1); 
}
