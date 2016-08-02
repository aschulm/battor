#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

#include "../interrupt.h"
#include "timer.h"
#include "led.h"

// ms timer
static volatile uint8_t sleep_ms_timer_fired = 0;
volatile uint32_t g_timer_ms_ticks = 0;

// real time clock
static uint32_t rtc_s = 0;
static uint32_t rtc_ms = 0;
static uint32_t rtc_prev_ms_ticks = 0;

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

uint32_t timer_elapsed_ms(uint32_t prev, uint32_t curr)
{
	if (prev > curr)
		return (0xFFFFFFFF - prev) + curr + 1;
	return curr - prev;
}

void timer_rtc_set(uint32_t secs)
{
	rtc_s = secs;
	rtc_ms = 0;
	rtc_prev_ms_ticks = g_timer_ms_ticks;
}

void timer_rtc_get(uint32_t* s, uint32_t* ms)
{
	timer_rtc_ms_update();

	*s = rtc_s;
	*ms = rtc_ms;

	printf("+++got time %lu %lu\n", *s, *ms);
}

void timer_rtc_ms_update()
{
	// if rtc has not been set, do not update it
	if (rtc_s <= 0)
		return;

	// udate rtc ms timer
	rtc_ms += timer_elapsed_ms(rtc_prev_ms_ticks, g_timer_ms_ticks);
	rtc_prev_ms_ticks = g_timer_ms_ticks;

	// update the seconds based on how many milliseconds have passed
	while (rtc_ms >= 1000)
	{
		rtc_s++;
		rtc_ms -= 1000;
	}
}
