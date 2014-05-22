#include "common.h"

#include "interrupt.h"

void halt() //{{{
{
	interrupt_disable();
	led_off(LED_GREEN_bm | LED_YELLOW_bm);
	led_on(LED_RED_bm);
	while (1) {}
} //}}}
