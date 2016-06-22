#include <avr/io.h>
#include <avr/interrupt.h>
#include "led.h"

#include "button.h"

ISR(PORTE_INT0_vect)
{
	led_toggle(LED_GREEN_bm);
}

void button_init()
{
	PORTE.DIR &= BUTTON_bm;
	// button requires a pullup
	PORTE.PIN0CTRL |= PORT_OPC_PULLUP_gc;
	// setup interrupt for button press
	PORTE.INTCTRL |= PORT_INT0LVL_HI_gc;
	PORTE.INT0MASK = BUTTON_bm;
}
