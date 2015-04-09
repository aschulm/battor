#include "common.h"

#include "control.h"
#include "blink.h"
#include "interrupt.h"

uint8_t g_error_last = 0;

void halt(uint8_t code) //{{{
{
	led_off(LED_GREEN_bm | LED_YELLOW_bm | LED_RED_bm);
	led_on(LED_RED_bm);

	// go back to idle mode and remember error
	g_control_mode = CONTROL_MODE_IDLE;
	g_error_last = code;
	dma_stop(); // stop getting samples from the ADCs

	/*blink_set_led(LED_RED_bm);
	blink_set_strobe_count(code);
	blink_set_interval_ms(5000);

	while (1) 
	{
		blink_ms_timer_update();
	}*/
} //}}}
