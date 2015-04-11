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
} INTERRUPT_t;

void interrupt_init();
void inline interrupt_set(uint16_t interrupt);
void inline interrupt_clear(uint16_t interrupt);
uint8_t interrupt_is_set(uint16_t interrupt);
void inline interrupt_disable();
void inline interrupt_enable();

#endif
