#ifndef GPIO_H
#define GPIO_H

#include <avr/io.h>

static inline void gpio_on(PORT_t* port, uint8_t pin)
{
	port->OUT |= pin;
}

static inline void gpio_off(PORT_t* port, uint8_t pin)
{
	port->OUT &= ~pin; 
}

static inline uint8_t gpio_read(PORT_t* port, uint8_t pin_p)
{
	uint8_t in = port->IN;
	return ((in >> pin_p) & 0x1); 
}

#endif
