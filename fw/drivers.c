#include "common.h"

#include "drivers.h"

void drivers_init() //{{{
{
	// ** order of init is very important **

	// ms timer for blinking led and other real time tasks
	printf("drivers_init: timer_init()\r\n");
	timer_init(&TCC0, TC_OVFINTLVL_LO_gc);
	printf("drivers_init: timer_set()\r\n");
	timer_set(&TCC0, TC_CLKSEL_DIV64_gc, 499);

	printf("drivers_init: led_init()\r\n");
	led_init();
	printf("drivers_init: mux_init()\r\n");
	mux_init();
	printf("drivers_init: uart_init()\r\n");
	uart_init();
	printf("drivers_init: pot_init()\r\n");
	pot_init();
	printf("drivers_init: adc_init(ADCA)\r\n");
	adc_init(&ADCA); // voltage ADC
	printf("drivers_init: adc_init(ADCB)\r\n");
	adc_init(&ADCB); // current ADC

	// sample timer
	timer_init(&TCD0,  TC_OVFINTLVL_OFF_gc);
	timer_set(&TCD0, TC_CLKSEL_DIV1024_gc, 0xFFFF);

	// gpio output for debugging
#ifdef GPIO_SAMPLE_WRITE_DONE
	PORTE.DIR |= (1<<GPIO_SAMPLE_WRITE_DONE);
#endif

#ifdef GPIO_DMA_INT
	PORTE.DIR |= (1<<GPIO_DMA_INT);
#endif
} //}}}
