#include "common.h"
#include "blink.h"

static uint16_t blink_ms; 
static uint16_t blink_interval_ms;
static uint16_t blink_prev_ms_ticks;
static uint8_t blink_led;

uint16_t ea(uint16_t p, uint16_t c) 
{
	if (p > c)
		return (0xFFFF - p) + c + 1;
	return c - p;
}

void blink_init(uint16_t interval_ms, uint8_t led)
{
	blink_ms = 0;
	blink_led = led;
	blink_interval_ms = interval_ms;
	blink_prev_ms_ticks = 0;
	led_off(led);
}

void blink_ms_timer_update()
{
	blink_ms += ea(blink_prev_ms_ticks, g_timer_ms_ticks);
	blink_prev_ms_ticks = g_timer_ms_ticks;

	if (blink_ms > blink_interval_ms)
		led_on(blink_led);
	if (blink_ms >= blink_interval_ms + BLINK_ON_TIME_MS)
	{
		led_off(blink_led);
		blink_ms = 0;
	}
}

void blink_set_led(uint8_t led)
{
	blink_led = led;
}

void blink_set_interval_ms(uint16_t interval_ms)
{
	blink_interval_ms = interval_ms;
	blink_ms = 0;
	blink_prev_ms_ticks = 0;
}
