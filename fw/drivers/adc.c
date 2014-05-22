#include <avr/io.h>
#include <stddef.h>
#include <stdio.h>

#include "adc.h"
#include "nvm.h"

void adc_init(ADC_t* adc)
{
	adc->CTRLB = (1<<4) | ADC_RESOLUTION_12BIT_gc; // 12bit, signed
	adc->REFCTRL = ADC_REFSEL_INT1V_gc | (1<<1); // defaults are good internal 1V reference, enable bandgap
	adc->PRESCALER = ADC_PRESCALER_DIV64_gc; // ADC clock max 2 MHz, be conservative to get higher precision 

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

	// setup the channel (signal)
	adc->CH0.CTRL = ADC_CH_INPUTMODE_DIFFWGAIN_gc;
	adc->CH0.MUXCTRL = ADC_CH_MUXPOS_PIN0_gc | ADC_CH_MUXNEG_PIN7_gc; 

	adc->EVCTRL = ADC_EVSEL_0123_gc | ADC_SWEEP_0_gc | ADC_EVACT_CH0_gc; // read channel 0 for event 0 

	adc->CTRLA = ADC_ENABLE_bm; // turn on the ADC
}
