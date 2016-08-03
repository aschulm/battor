#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

#include "../control.h"
#include "timer.h"
#include "button.h"

static uint16_t down_ms;
static uint32_t prev_ms_ticks;
static uint8_t button_state;
typedef enum BUTTON_STATE_enum
{
	BUTTON_STATE_IDLE,
	BUTTON_STATE_HOLDING,
	BUTTON_STATE_HELD,
} BUTTON_STATE_t;

ISR(PORTE_INT0_vect) //{{{
{
	// set time that button went down
	prev_ms_ticks = g_timer_ms_ticks;
} //}}}

void button_init()
{
	PORTE.DIR &= ~BUTTON_bm;
	// button requires a pullup and is falling edge triggered
	PORTE.PIN0CTRL = PORT_OPC_PULLUP_gc | PORT_ISC_FALLING_gc;
	// enable interrupts
	PORTE.INTCTRL |= PORT_INT0LVL_HI_gc;
	PORTE.INT0MASK |= BUTTON_bm;

	down_ms = 0;
	prev_ms_ticks = 0;

	button_state = BUTTON_STATE_IDLE;
}

void button_ms_timer_update()
{
	uint8_t button_is_down = ((PORTE.IN & BUTTON_bm) == 0);
	switch (button_state)
	{
		case BUTTON_STATE_IDLE:
			if (button_is_down)
			{
				down_ms = 0;
				prev_ms_ticks = g_timer_ms_ticks;
				button_state = BUTTON_STATE_HOLDING;
			}
		break;
		case BUTTON_STATE_HOLDING:
			if (button_is_down)
			{
				down_ms += timer_elapsed_ms(prev_ms_ticks, g_timer_ms_ticks);
				prev_ms_ticks = g_timer_ms_ticks;

				if (down_ms >= BUTTON_HOLD_MS)
				{
					control_button_hold();
					button_state = BUTTON_STATE_HELD;
				}
			}
			else
			{
				if (down_ms < BUTTON_HOLD_MS)
				{
					control_button_press();
					button_state = BUTTON_STATE_IDLE;
				}
			}
		break;
		case BUTTON_STATE_HELD:
			if (!button_is_down)
				button_state = BUTTON_STATE_IDLE;
		break;
	}
}

int button_self_test()
{
	printf("button self test\n");
	printf("waiting for button press...");
	while ((PORTE.IN & BUTTON_bm) > 0);
	printf("PASSED\n");
	return 0;
}
