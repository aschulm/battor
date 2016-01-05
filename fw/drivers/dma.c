#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <string.h>

#include "../error.h"
#include "../interrupt.h"
#include "adc.h"
#include "timer.h"
#include "dma.h"
#include "gpio.h"

static uint8_t uart_in_progress;

ISR(DMA_CH2_vect)
{
#ifdef GPIO_DMA_INT
	gpio_toggle(&PORTE, (1<<GPIO_DMA_INT));
#endif

	if (interrupt_is_set(INTERRUPT_DMA_CH2))
		halt(ERROR_DMA_CH2_OVERFLOW);
	interrupt_set(INTERRUPT_DMA_CH2);
	DMA.INTFLAGS = DMA_CH2TRNIF_bm; // clear the interrupt
}
ISR(DMA_CH3_vect)
{
#ifdef GPIO_DMA_INT
	gpio_toggle(&PORTE, (1<<GPIO_DMA_INT));
#endif

	if (interrupt_is_set(INTERRUPT_DMA_CH3))
		halt(ERROR_DMA_CH3_OVERFLOW);
	interrupt_set(INTERRUPT_DMA_CH3);
	DMA.INTFLAGS = DMA_CH3TRNIF_bm; // clear the interrupt
}

void set_24_bit_addr(volatile uint8_t* d, uint16_t v)
{
	d[0] = (v & 0x00FF) >> 0;
	d[1] = (v & 0xFF00) >> 8;
	d[2] = 0;
}

void dma_init()
{
	// reset the DMA controller
	DMA.CTRL = 0;
	DMA.CTRL = DMA_RESET_bm;
	loop_until_bit_is_clear(DMA.CTRL, DMA_RESET_bp);
	
	DMA.CTRL = DMA_ENABLE_bm | DMA_DBUFMODE_CH23_gc | DMA_PRIMODE_RR0123_gc; // enable DMA, double buffer 2,3 (ADCB)

	// reset UART transaction status
	uart_in_progress = 0;
}

void dma_start(void* adcb_samples0, void* adcb_samples1, uint16_t samples_len)
{
	// clear the buffers
	memset(adcb_samples0, 0, samples_len);
	memset(adcb_samples1, 0, samples_len);

	// repeat forevr
	DMA.CH2.REPCNT = 0;
	DMA.CH3.REPCNT = 0;

	// setup the channels to copy 4 byte bursts (ADC CH2 + CH3) and one burst blocks, trigger only fires burst
	DMA.CH2.CTRLA = DMA_CH_REPEAT_bm | DMA_CH_BURSTLEN_4BYTE_gc | DMA_CH_SINGLE_bm;
	DMA.CH3.CTRLA = DMA_CH_REPEAT_bm | DMA_CH_BURSTLEN_4BYTE_gc | DMA_CH_SINGLE_bm;

	// reload the ADC addr every burst, increment the source, reload dst every transaction and increment it
	DMA.CH2.ADDRCTRL = DMA_CH_SRCRELOAD_BURST_gc | DMA_CH_SRCDIR_INC_gc | DMA_CH_DESTRELOAD_BLOCK_gc | DMA_CH_DESTDIR_INC_gc;
	DMA.CH3.ADDRCTRL = DMA_CH_SRCRELOAD_BURST_gc | DMA_CH_SRCDIR_INC_gc | DMA_CH_DESTRELOAD_BLOCK_gc | DMA_CH_DESTDIR_INC_gc;

	// trigger on ADC channel 1 (sweep ends there, starts at 0)
	DMA.CH2.TRIGSRC = DMA_CH_TRIGSRC_ADCB_CH1_gc;
	DMA.CH3.TRIGSRC = DMA_CH_TRIGSRC_ADCB_CH1_gc;

	// 4 bytes per block transfter (ADC CH2 + CH3 16bit values)
	DMA.CH2.TRFCNT = samples_len;
	DMA.CH3.TRFCNT = samples_len;

	// setup the source addr to result 0 in the respective ADCs
	set_24_bit_addr(&(DMA.CH2.SRCADDR0), (uint16_t)&(ADCB.CH0RES));
	set_24_bit_addr(&(DMA.CH3.SRCADDR0), (uint16_t)&(ADCB.CH0RES));

	// setup the dst addr to the passed in buffers
	set_24_bit_addr(&(DMA.CH2.DESTADDR0), (uint16_t)adcb_samples0);
	set_24_bit_addr(&(DMA.CH3.DESTADDR0), (uint16_t)adcb_samples1);

	DMA.INTFLAGS = 0xFF; // clear all interrups
	DMA.CH2.CTRLA |= DMA_CH_ENABLE_bm; // start DMA channel 2 for ADCB, will auto double buffer with channel 3

	// interrupt when the transaction is complete
	DMA.CH2.CTRLB = DMA_CH_TRNINTLVL_MED_gc;
	DMA.CH3.CTRLB = DMA_CH_TRNINTLVL_MED_gc;

	EVSYS.CH0MUX = EVSYS_CHMUX_TCD0_OVF_gc; // event channel 0 will fire when TCD0 overflows
}

void inline dma_pause(uint8_t on_off)
{
	if (on_off)
	{
		DMA.CH2.TRIGSRC = DMA_CH_TRIGSRC_OFF_gc;
		DMA.CH3.TRIGSRC = DMA_CH_TRIGSRC_OFF_gc;
	}
	else
	{
		DMA.CH2.TRIGSRC = DMA_CH_TRIGSRC_ADCB_CH1_gc;
		DMA.CH3.TRIGSRC = DMA_CH_TRIGSRC_ADCB_CH1_gc;
	}
}

void dma_stop()
{
	DMA.CH2.CTRLA &= ~DMA_CH_ENABLE_bm; // stop DMA channel 2 for ADCB
	DMA.CH3.CTRLA &= ~DMA_CH_ENABLE_bm; // stop DMA channel 2 for ADCB

	DMA.INTFLAGS = 0xFF; // clear all interrups

	EVSYS.CH0MUX = 0; // stop event channel 0
}

void dma_uart_tx(const void* data, uint16_t len)
{
	// clear transfer complete flag, needed to wait for last byte
	DMA.CH0.CTRLB = DMA_CH_TRNIF_bm;
	USARTD0.STATUS = USART_TXCIF_bm;

	DMA.CH0.CTRLA = DMA_CH_BURSTLEN_1BYTE_gc | DMA_CH_SINGLE_bm;
	DMA.CH0.ADDRCTRL = DMA_CH_SRCRELOAD_NONE_gc | DMA_CH_SRCDIR_INC_gc | DMA_CH_DESTRELOAD_BURST_gc | DMA_CH_DESTDIR_FIXED_gc;
	DMA.CH0.TRIGSRC = DMA_CH_TRIGSRC_USARTD0_DRE_gc;
	DMA.CH0.TRFCNT = len;
	set_24_bit_addr(&(DMA.CH0.SRCADDR0), (uint16_t)(data));
	set_24_bit_addr(&(DMA.CH0.DESTADDR0), (uint16_t)&(USARTD0.DATA));
	DMA.CH0.CTRLA |= DMA_CH_ENABLE_bm;
	uart_in_progress = 1;
}

void dma_uart_tx_pause(uint8_t on_off)
{
	if (on_off)
		DMA.CH0.TRIGSRC = DMA_CH_TRIGSRC_OFF_gc;
	else
		DMA.CH0.TRIGSRC = DMA_CH_TRIGSRC_USARTD0_DRE_gc;
}

uint8_t dma_uart_tx_ready()
{
	// no DMA transaction started so it is ready
	if (!uart_in_progress)
		return 1;

	if ((DMA.CH0.CTRLB & DMA_CH_TRNIF_bm) > 0 &&
		(USARTD0.STATUS & USART_TXCIF_bm) > 0)
	{
		DMA.CH0.CTRLB = DMA_CH_TRNIF_bm; // clear flag
		uart_in_progress = 0;
		return 1;
	}

	return 0;
}

void dma_spi_txrx(USART_t* usart, const void* txd, void* rxd, uint16_t len)
{
	if (rxd != NULL)
	{
		// clear out all rx data
		while ((usart->STATUS & USART_RXCIF_bm) > 0)
			usart->DATA;

		// setup receive DMA
		DMA.CH0.CTRLB = DMA_CH_TRNIF_bm; // clear flag
		DMA.CH0.CTRLA = DMA_CH_BURSTLEN_1BYTE_gc | DMA_CH_SINGLE_bm;
		DMA.CH0.ADDRCTRL = DMA_CH_SRCRELOAD_BURST_gc |
			DMA_CH_SRCDIR_FIXED_gc |
			DMA_CH_DESTRELOAD_NONE_gc |
			DMA_CH_DESTDIR_INC_gc;
		if (usart == &USARTC1)
			DMA.CH0.TRIGSRC = DMA_CH_TRIGSRC_USARTC1_RXC_gc;
		if (usart == &USARTE1)
			DMA.CH0.TRIGSRC = DMA_CH_TRIGSRC_USARTE1_RXC_gc;
		DMA.CH0.TRFCNT = len;
		set_24_bit_addr(&(DMA.CH0.SRCADDR0), (uint16_t)&(usart->DATA));
		set_24_bit_addr(&(DMA.CH0.DESTADDR0), (uint16_t)(rxd));
		DMA.CH0.CTRLA |= DMA_CH_ENABLE_bm;
	}

	uint8_t ff = 0xFF;

	// clear flag, needed to wait for last byte
	usart->STATUS = USART_TXCIF_bm;

	// setup transmit DMA
	DMA.CH1.CTRLB = DMA_CH_TRNIF_bm; // clear flag
	DMA.CH1.CTRLA = DMA_CH_BURSTLEN_1BYTE_gc | DMA_CH_SINGLE_bm;

	DMA.CH1.ADDRCTRL = DMA_CH_DESTRELOAD_BURST_gc |
		DMA_CH_DESTDIR_FIXED_gc;

	// either write bytes or just 0xFF
	if (txd != NULL)
	{
		DMA.CH1.ADDRCTRL |= DMA_CH_SRCRELOAD_NONE_gc |
			DMA_CH_SRCDIR_INC_gc;
		set_24_bit_addr(&(DMA.CH1.SRCADDR0), (uint16_t)(txd));
	}
	else
	{
		DMA.CH1.ADDRCTRL |= DMA_CH_SRCRELOAD_BURST_gc |
			DMA_CH_SRCDIR_FIXED_gc;
		set_24_bit_addr(&(DMA.CH1.SRCADDR0), (uint16_t)(&ff));
	}

	if (usart == &USARTC1)
		DMA.CH1.TRIGSRC = DMA_CH_TRIGSRC_USARTC1_DRE_gc;
	if (usart == &USARTE1)
		DMA.CH1.TRIGSRC = DMA_CH_TRIGSRC_USARTE1_DRE_gc;
	DMA.CH1.TRFCNT = len;
	set_24_bit_addr(&(DMA.CH1.DESTADDR0), (uint16_t)&(usart->DATA));
	DMA.CH1.CTRLA |= DMA_CH_ENABLE_bm;

	// wait for transfer to complete
	loop_until_bit_is_set(DMA.CH1.CTRLB, DMA_CH_TRNIF_bp);
	if (rxd != NULL)
		loop_until_bit_is_set(DMA.CH0.CTRLB, DMA_CH_TRNIF_bp);

	// wait for last byte
	loop_until_bit_is_set(usart->STATUS, USART_TXCIF_bp);
	usart->STATUS = USART_TXCIF_bm;
}
