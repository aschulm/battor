#include "common.h"
#include "error.h"
#include "interrupt.h"

static volatile uint16_t g_interrupt;

void interrupt_init() //{{{
{
	g_interrupt = 0; // clear all interrupts
	int_enable(PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm); // enable the low priority interrupts
	sei(); // enable interrupts
} //}}}

void inline interrupt_set(uint16_t interrupt) //{{{
{
	g_interrupt |= interrupt;
} //}}}

void inline interrupt_clear(uint16_t interrupt) //{{{
{
	g_interrupt &= ~interrupt;
} //}}}

uint8_t interrupt_is_set(uint16_t interrupt) //{{{
{
	return (g_interrupt & interrupt) > 0;
} //}}}

void inline interrupt_disable() //{{{
{
	cli();	
} //}}}

void inline interrupt_enable() //{{{
{
	sei();	
} //}}}
