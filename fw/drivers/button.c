#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

#include "../interrupt.h"
#include "../control.h"
#include "timer.h"
#include "button.h"

static uint16_t down_ms;
static uint16_t prev_ms_ticks;


ISR(PORTE_INT0_vect) //{{{
{
	// set time that button went down
	prev_ms_ticks = g_timer_ms_ticks;
} //}}}

void button_init()
{
	PORTE.DIR &= ~BUTTON_bm;
	// button requires a pullup and is negative edge triggered
	PORTE.PIN0CTRL = PORT_OPC_PULLUP_gc | PORT_ISC_FALLING_gc;

	PORTE.INTCTRL |= PORT_INT0LVL_HI_gc;
	PORTE.INT0MASK |= BUTTON_bm;

	down_ms = 0;
	prev_ms_ticks = 0;
}

void button_ms_timer_update()
{
	// update down timer
	if ((PORTE.IN & BUTTON_bm) == 0)
	{
		down_ms += timer_elapsed_ms(prev_ms_ticks, g_timer_ms_ticks);
		prev_ms_ticks = g_timer_ms_ticks;
	}
	else
	{
		// not down anymore and was down for less than hold time
		if (down_ms > 0 && down_ms < BUTTON_HOLD_MS)
			control_button_press();

		// reset down timer
		down_ms = 0;
	}

	// button is being held
	if (down_ms >= BUTTON_HOLD_MS)
		control_button_hold();
}

int button_self_test()
{
	printf("button self test\n");
	printf("waiting for button press...");
	while ((PORTE.IN & BUTTON_bm) > 0);
	printf("PASSED\n");
	return 0;
}
