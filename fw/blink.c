#include "common.h"
#include "blink.h"

static uint16_t blink_ms, blink_strobe_ms; 
static uint16_t blink_interval_ms, blink_strobe_count;
static uint16_t blink_prev_ms_ticks;
static uint8_t blink_led;
static uint8_t blink_first;

void blink_init(uint16_t interval_ms, uint8_t led)
{
	blink_ms = 0;
	blink_strobe_ms = 0;
	blink_led = led;
	blink_interval_ms = interval_ms;
	blink_strobe_count = 1;
	blink_prev_ms_ticks = 0;
	blink_first = 1;
	led_off(led);
}

void blink_ms_timer_update()
{
	// if first run then blink immediatly, else wait as it should
	if (blink_first)
	{
		blink_ms = blink_interval_ms;
		blink_strobe_ms = BLINK_ON_TIME_MS;
		blink_prev_ms_ticks = g_timer_ms_ticks;
		blink_first = 0;
	}
	else
	{
		blink_ms += timer_elapsed_ms(blink_prev_ms_ticks, g_timer_ms_ticks);
		blink_strobe_ms += timer_elapsed_ms(blink_prev_ms_ticks, g_timer_ms_ticks);
		blink_prev_ms_ticks = g_timer_ms_ticks;
	}

	if (blink_ms >= blink_interval_ms)
	{
		// strobe
		if (blink_strobe_ms >= BLINK_ON_TIME_MS)
		{
			led_toggle(blink_led);
			blink_strobe_ms = 0;
		}
	}
	if (blink_ms >= blink_interval_ms + (BLINK_ON_TIME_MS * (blink_strobe_count*2)))
	{
		// end strobe
		led_off(blink_led);
		blink_ms = 0;
	}
}

void blink_set_led(uint8_t led)
{
	// turn of old blink led
	led_off(blink_led);

	blink_led = led;
}

void blink_set_interval_ms(uint16_t interval_ms)
{
	blink_interval_ms = interval_ms;
}

void blink_set_strobe_count(uint16_t count)
{
	blink_strobe_count = count;		
}
