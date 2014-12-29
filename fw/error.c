#include "common.h"

#include "blink.h"
#include "interrupt.h"

void halt(uint8_t code) //{{{
{
	led_off(LED_GREEN_bm | LED_YELLOW_bm | LED_RED_bm);
	led_on(LED_RED_bm);
	while(1);

	/*blink_set_led(LED_RED_bm);
	blink_set_strobe_count(code);
	blink_set_interval_ms(5000);

	while (1) 
	{
		blink_ms_timer_update();
	}*/
} //}}}
