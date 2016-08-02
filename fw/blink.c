#include "common.h"
#include "blink.h"

static uint16_t blink_ms, blink_strobe_ms; 
static uint16_t blink_interval_ms, blink_strobe_count, blink_strobe_idx;
static uint32_t blink_prev_ms_ticks, blink_strobe_prev_ms_ticks;
static uint8_t blink_led;
static uint8_t first_blink;
static uint8_t strobe_state;
typedef enum STROBE_STATE_enum
{
		STROBE_STATE_FIRST,
		STROBE_STATE_WAIT_FOR_LED_ON,
		STROBE_STATE_LED_ON,
		STROBE_STATE_LED_OFF,
} STROBE_STATE_t;

void blink_init(uint16_t interval_ms, uint8_t led)
{
	blink_led = led;
	blink_interval_ms = interval_ms;
	blink_ms = 0;
	blink_strobe_ms = 0;
	blink_strobe_count = 1;
	blink_strobe_idx = 0;
	blink_prev_ms_ticks = 0;
	blink_strobe_prev_ms_ticks = 0;
	first_blink = 1;
	led_off(led);
}

void blink_ms_timer_update()
{
	if (first_blink)
	{
		blink_ms = blink_interval_ms;
		blink_prev_ms_ticks = g_timer_ms_ticks;
		blink_strobe_ms = 0;
		strobe_state = STROBE_STATE_FIRST;
		first_blink = 0;
	}
	blink_ms += timer_elapsed_ms(blink_prev_ms_ticks, g_timer_ms_ticks);
	blink_prev_ms_ticks = g_timer_ms_ticks;

	if (blink_ms >= blink_interval_ms)
	{
		if (strobe_state == STROBE_STATE_FIRST)
		{
			blink_strobe_prev_ms_ticks = g_timer_ms_ticks;
			blink_strobe_idx = 0;
			strobe_state = STROBE_STATE_WAIT_FOR_LED_ON;
		}
		blink_strobe_ms += timer_elapsed_ms(blink_strobe_prev_ms_ticks, g_timer_ms_ticks);
		blink_strobe_prev_ms_ticks = g_timer_ms_ticks;

		if (blink_strobe_idx < blink_strobe_count)
		{
			switch (strobe_state)
			{
				case STROBE_STATE_WAIT_FOR_LED_ON:
					led_on(blink_led);
					blink_strobe_ms = 0;
					strobe_state = STROBE_STATE_LED_ON;
				break;
				case STROBE_STATE_LED_ON:
					if (blink_strobe_ms >= BLINK_STROBE_ON_TIME_MS)
					{
						led_off(blink_led);
						blink_strobe_ms = 0;
						strobe_state = STROBE_STATE_LED_OFF;
					}
				break;
				case STROBE_STATE_LED_OFF:
					if (blink_strobe_ms >= BLINK_STROBE_OFF_TIME_MS)
					{
						strobe_state = STROBE_STATE_WAIT_FOR_LED_ON;
						blink_strobe_idx++;
					}
				break;
			}
		}
		else
		{
			blink_ms = 0;
			strobe_state = STROBE_STATE_FIRST;
		}
	}
}

void blink_set_led(uint8_t led)
{
	// turn of old blink led
	led_off(blink_led);
	blink_led = led;
	first_blink = 1;
}

void blink_set_interval_ms(uint16_t interval_ms)
{
	blink_interval_ms = interval_ms;
	first_blink = 1;
}

void blink_set_strobe_count(uint16_t count)
{
	if (count > 0)
	{
		blink_strobe_count = count;
		first_blink = 1;
	}
}
