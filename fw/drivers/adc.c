#include <avr/io.h>
#include <stddef.h>
#include <stdio.h>

#include "adc.h"
#include "nvm.h"
#include "timer.h"

#define ADC_CAL_NUM 100

void adc_init(ADC_t* adc)
{
	adc->CTRLB = ADC_CONMODE_bm | ADC_RESOLUTION_12BIT_gc; // 12bit, signed (sadly, have to do signed and lose a bit since differential has much less noise and it's only available in signed)
	adc->CTRLB |= ADC_IMPMODE_bm; // use low impedience mode because input sources are not high impedance and do not need charge to remain in a capacitor
	adc->REFCTRL = ADC_REFSEL_AREFA_gc; // external reference (1.200 V) on PORTA
	adc->PRESCALER = ADC_PRESCALER_DIV64_gc; // set ADC clock to less than 2 MHz

	// get the factory written ADC calibration value
	uint16_t adc_cal = 0;
	if (adc == &ADCA)
	{
		adc_cal =  nvm_read_calibration_byte(offsetof(NVM_PROD_SIGNATURES_t, ADCACAL0));
		adc_cal |= nvm_read_calibration_byte(offsetof(NVM_PROD_SIGNATURES_t, ADCACAL1)) << 8;
	}
	if (adc == &ADCB)
	{
		adc_cal =  nvm_read_calibration_byte(offsetof(NVM_PROD_SIGNATURES_t, ADCBCAL0));
		adc_cal |= nvm_read_calibration_byte(offsetof(NVM_PROD_SIGNATURES_t, ADCBCAL1)) << 8; 
	}
	adc->CAL = adc_cal; // load the calibration value into the ADC adc_cal

	// setup voltage measurement channel 
	adc->CH0.CTRL = ADC_CH_INPUTMODE_DIFFWGAIN_gc; // 1x gain needed due to high impedience
	adc->CH0.MUXCTRL = ADC_CH_MUXPOS_PIN0_gc | ADC_CH_MUXNEG_GND_MODE4_gc;

	// setup current measurement channel
	adc->CH1.CTRL = ADC_CH_INPUTMODE_DIFF_gc; // gain needed to use GND 
	adc->CH1.MUXCTRL = ADC_CH_MUXPOS_PIN1_gc | ADC_CH_MUXNEG_GND_MODE3_gc;

	// setup interrupts
	adc->CH0.INTCTRL = ADC_CH_INTMODE_COMPLETE_gc | ADC_CH_INTLVL_OFF_gc;
	adc->CH1.INTCTRL = ADC_CH_INTMODE_COMPLETE_gc | ADC_CH_INTLVL_OFF_gc;

	adc->EVCTRL = ADC_EVSEL_0123_gc | ADC_SWEEP_01_gc | ADC_EVACT_SYNCSWEEP_gc; // read channel 0 for event 0 

	adc->CTRLA = ADC_DMASEL_CH01_gc | ADC_ENABLE_bm; // turn on the ADC

	// wait for the adc to settle
	timer_sleep_ms(10);
}

int16_t adc_sample(ADC_t* adc, uint8_t channel)
{
	adc->CTRLA |= (ADC_CH0START_bm << channel);
	loop_until_bit_is_set(adc->INTFLAGS, channel);
	return *(&adc->CH0RES + channel);
}
