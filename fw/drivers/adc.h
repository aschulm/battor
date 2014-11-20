#ifndef ADC_H
#define ADC_H

void adc_init(ADC_t* adc);
int16_t adc_sample(ADC_t* adc, uint8_t channel);

#endif
