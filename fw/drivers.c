#include "common.h"

#include "drivers.h"

void drivers_init() //{{{
{
	// ** order of init is very important **
	
	printf("drivers_init: uart_init()\n");
	uart_init();

	// ms timer for blinking led and other real time tasks
	printf("drivers_init: timer_init()\n");
	timer_init(&TCC0, TC_OVFINTLVL_LO_gc);
	printf("drivers_init: timer_set()\n");
	timer_set(&TCC0, TC_CLKSEL_DIV64_gc, 499);

	printf("drivers_init: dma_init()\n");
	dma_init();

	printf("drivers_init: mux_init()\n");
	mux_init();
	printf("drivers_init: pot_init()\n");
	pot_init();
	printf("drivers_init: sram_init()\n");
	sram_init();
	printf("drivers_init: adc_init(ADCB)\n");
	adc_init(&ADCB);

	// sample timer
	timer_init(&TCD0,  TC_OVFINTLVL_OFF_gc);
	timer_set(&TCD0, TC_CLKSEL_DIV1024_gc, 0xFFFF);

	printf("drivers_init: led_init()\n");
	led_init();

	// gpio outputs for debugging
#ifdef GPIO_SAMPLE_WRITE_DONE
	PORTE.DIR |= (1<<GPIO_SAMPLE_WRITE_DONE);
#endif
#ifdef GPIO_DMA_INT
	PORTE.DIR |= (1<<GPIO_DMA_INT);
#endif
#ifdef GPIO_MAIN_LOOP
	PORTE.DIR |= (1<<GPIO_MAIN_LOOP);
#endif
} //}}}

int drivers_self_test()
{
	if (pot_self_test())
		return 1;
	if (sram_self_test())
		return 2;
	return 0;
}
