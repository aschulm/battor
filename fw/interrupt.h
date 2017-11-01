#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <avr/interrupt.h>

extern volatile uint16_t g_interrupt;

typedef enum INTERRUPT_enum
{
	INTERRUPT_TIMER_MS = (1<<0),
	INTERRUPT_UART_RX = (1<<1),
	INTERRUPT_DMA_CH0 = (1<<2),
	INTERRUPT_DMA_CH1 = (1<<3),
	INTERRUPT_DMA_CH2 = (1<<4),
	INTERRUPT_DMA_CH3 = (1<<5),
} INTERRUPT_t;

void interrupt_init();

void inline interrupt_set(uint16_t interrupt) //{{{
{
	g_interrupt |= interrupt;
} //}}}

void inline interrupt_clear(uint16_t interrupt) //{{{
{
	g_interrupt &= ~interrupt;
} //}}}

void inline interrupt_disable() //{{{
{
	cli();
} //}}}

void inline interrupt_enable() //{{{
{
	sei();
} //}}}

uint8_t inline interrupt_is_set(uint16_t interrupt) //{{{
{
	return (g_interrupt & interrupt) > 0;
} //}}}

#endif
