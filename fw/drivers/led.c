#include <avr/io.h>

#include "led.h"

void led_init()
{
	PORTC.OUT |= LED_RED_bm | LED_YELLOW_bm | LED_GREEN_bm; // all leds off (leds are active low)
	PORTC.DIR |= LED_RED_bm | LED_YELLOW_bm | LED_GREEN_bm; // ports set to output
}

void led_on(char led)
{
	PORTC.OUT &= ~led;
}

void led_off(char led)
{
	PORTC.OUT |= led;
}

void led_toggle(char led)
{
	PORTC.OUT ^= led;
}

int led_self_test()
{
	led_on(LED_RED_bm | LED_YELLOW_bm | LED_GREEN_bm);
	return 0;
}
