#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

#include "../interrupt.h"
#include "timer.h"
#include "led.h"

// ms timer
static volatile uint8_t sleep_ms_timer_fired = 0;
volatile uint16_t g_timer_ms_ticks = 0;

// sec real time clock
uint32_t rtc_s = 0;

ISR(TCC0_OVF_vect)
{
	g_timer_ms_ticks++;
	sleep_ms_timer_fired = 1;
	interrupt_set(INTERRUPT_TIMER_MS);
}

void timer_init(TC0_t* timer, uint8_t int_level)
{
	timer->CTRLA = 0; // set prescaler later
	timer->CTRLB = 0; // not used
	timer->CTRLC = 0; // not used
	timer->CTRLD = 0; // not used
	timer->CTRLE = 0; // not used
	timer->CNT = 0; // reset the timer before enabling interrupts
	timer->INTCTRLA = int_level; // set the interrupt level for this timer
	timer->INTCTRLB = 0;
}

void timer_reset(TC0_t* timer)
{
	timer->CNT = 0;
}

void timer_set(TC0_t* timer, uint8_t prescaler, uint16_t period)
{
	timer->CTRLA = prescaler; // set the clock divider
	timer->PER = period;
}

void timer_sleep_ms(uint16_t ms)
{
	uint16_t ms_asleep = 0;
	while (ms_asleep < ms)
	{
		while (!sleep_ms_timer_fired); // wait for timer to fire
		sleep_ms_timer_fired = 0; // saw the timer fire
		ms_asleep++;
	}
}

uint16_t timer_elapsed_ms(uint16_t prev, uint16_t curr)
{
	if (prev > curr)
		return (0xFFFF - prev) + curr + 1;
	return curr - prev;
}

void timer_rtc_set(uint32_t secs)
{
	rtc_s = secs;
}

void timer_rtc_update_ms
