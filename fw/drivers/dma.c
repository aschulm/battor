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

ISR(DMA_CH2_vect)
{
	if (interrupt_is_set(INTERRUPT_DMA_CH2))
		halt(ERROR_DMA_CH2_OVERFLOW);
	interrupt_set(INTERRUPT_DMA_CH2);
	DMA.INTFLAGS = DMA_CH2TRNIF_bm; // clear the interrupt
}
ISR(DMA_CH3_vect)
{
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

void dma_init(void* adcb_samples0, void* adcb_samples1, uint16_t samples_len)
{
	// clear the buffers
	memset(adcb_samples0, 0, samples_len);
	memset(adcb_samples1, 0, samples_len);

	// reset the DMA controller
	DMA.CTRL = 0;
	DMA.CTRL = DMA_RESET_bm;
	loop_until_bit_is_clear(DMA.CTRL, DMA_RESET_bp);
	
	DMA.CTRL = DMA_ENABLE_bm | DMA_DBUFMODE_CH23_gc | DMA_PRIMODE_RR0123_gc; // enable DMA, double buffer 2,3 (ADCB)

	// repeat forevr
	DMA.CH2.REPCNT = 0;
	DMA.CH3.REPCNT = 0;

	// setup the channels to copy 4 byte bursts (ADC CH0 + CH1) and one burst blocks, trigger only fires burst
	DMA.CH2.CTRLA = DMA_CH_REPEAT_bm | DMA_CH_BURSTLEN_4BYTE_gc | DMA_CH_SINGLE_bm;
	DMA.CH3.CTRLA = DMA_CH_REPEAT_bm | DMA_CH_BURSTLEN_4BYTE_gc | DMA_CH_SINGLE_bm;

	// reload the ADC addr every burst, increment the source, reload dst every transaction and increment it
	DMA.CH2.ADDRCTRL = DMA_CH_SRCRELOAD_BURST_gc | DMA_CH_SRCDIR_INC_gc | DMA_CH_DESTRELOAD_BLOCK_gc | DMA_CH_DESTDIR_INC_gc;
	DMA.CH3.ADDRCTRL = DMA_CH_SRCRELOAD_BURST_gc | DMA_CH_SRCDIR_INC_gc | DMA_CH_DESTRELOAD_BLOCK_gc | DMA_CH_DESTDIR_INC_gc;

	// trigger on ADC channel 1 (sweep ends there, starts at 0)
	DMA.CH2.TRIGSRC = DMA_CH_TRIGSRC_ADCB_CH1_gc;
	DMA.CH3.TRIGSRC = DMA_CH_TRIGSRC_ADCB_CH1_gc;

	// 4 bytes per block transfter (ADC CH0 + CH1 16bit values)
	DMA.CH2.TRFCNT = samples_len;
	DMA.CH3.TRFCNT = samples_len;

	// setup the source addr to result 0 in the respective ADCs
	set_24_bit_addr(&(DMA.CH2.SRCADDR0), (uint16_t)&(ADCB.CH0RES));
	set_24_bit_addr(&(DMA.CH3.SRCADDR0), (uint16_t)&(ADCB.CH0RES));

	// setup the dst addr to the passed in buffers
	set_24_bit_addr(&(DMA.CH2.DESTADDR0), (uint16_t)adcb_samples0);
	set_24_bit_addr(&(DMA.CH3.DESTADDR0), (uint16_t)adcb_samples1);
}

void dma_start()
{
	DMA.INTFLAGS = 0xFF; // clear all interrups
	DMA.CH0.CTRLA |= DMA_CH_ENABLE_bm; // start DMA channel 0 for ADCB, will auto double buffer with channel 1
	DMA.CH2.CTRLA |= DMA_CH_ENABLE_bm; // start DMA channel 2 for ADCB, will auto double buffer with channel 3

	// interrupt when the transaction is complete
	DMA.CH2.CTRLB = DMA_CH_TRNINTLVL_MED_gc;
	DMA.CH3.CTRLB = DMA_CH_TRNINTLVL_MED_gc;

	EVSYS.CH0MUX = EVSYS_CHMUX_TCD0_OVF_gc; // event channel 0 will fire when TCD0 overflows
}

void dma_stop()
{
	DMA.CH2.CTRLA &= ~DMA_CH_ENABLE_bm; // stop DMA channel 2 for ADCB
	DMA.CH3.CTRLA &= ~DMA_CH_ENABLE_bm; // stop DMA channel 2 for ADCB

	DMA.INTFLAGS = 0xFF; // clear all interrups

	EVSYS.CH0MUX = 0; // stop event channel 0
}
