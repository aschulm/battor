#include "common.h"

#include "control.h"
#include "blink.h"
#include "interrupt.h"
#include "drivers.h"

uint8_t g_error_last __attribute__ ((section (".noinit"))); // do not zero out on reset

void halt(uint8_t code) //{{{
{
	interrupt_disable();
	led_off(LED_GREEN_bm | LED_YELLOW_bm | LED_RED_bm);
	led_on(LED_RED_bm);

	// remember last error
	g_error_last = code;
	
	reset();

	/*blink_set_led(LED_RED_bm);
	blink_set_strobe_count(code);
	blink_set_interval_ms(5000);

	while (1) 
	{
		blink_ms_timer_update();
	}*/
} //}}}
