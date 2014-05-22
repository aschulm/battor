#ifndef ADC_H
#define ADC_H

void adc_init(ADC_t* adc);

typedef struct adc_sample_t
{
	uint16_t gnd;
	uint16_t signal;
} adc_sample;

#endif
