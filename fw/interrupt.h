#ifndef INTERRUPT_H
#define INTERRUPT_H

typedef enum INTERRUPT_enum
{
	INTERRUPT_TIMER_MS = (1<<0),
	INTERRUPT_UART_RX = (1<<1),
	INTERRUPT_DMA_CH0 = (1<<2),
	INTERRUPT_DMA_CH1 = (1<<3),
	INTERRUPT_DMA_CH2 = (1<<4),
	INTERRUPT_DMA_CH3 = (1<<5),
	INTERRUPT_GPIO = (1<<6),
} INTERRUPT_t;

extern volatile uint16_t g_interrupt;

void interrupt_init();
uint8_t interrupt_is_set(uint16_t interrupt);

static inline void interrupt_set(uint16_t interrupt) //{{{
{
	g_interrupt |= interrupt;
} //}}}

static inline void interrupt_clear(uint16_t interrupt) //{{{
{
	g_interrupt &= ~interrupt;
} //}}}

static inline void interrupt_disable() //{{{
{
	cli();	
} //}}}

static inline void interrupt_enable() //{{{
{
	sei();	
} //}}}

#endif
