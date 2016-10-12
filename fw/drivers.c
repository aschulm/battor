#include "common.h"

#include "drivers.h"

void drivers_init() //{{{
{
	// ** order of init is very important **
	uart_init();

	// ms timer for blinking led and other real time tasks
	timer_init(&TCC0, TC_OVFINTLVL_LO_gc);
	timer_set(&TCC0, TC_CLKSEL_DIV64_gc, 499);

	dma_init(0);
	mux_init();
	pot_init();
	sram_init();
	adc_init(&ADCB);
	sd_init();

	// sample timer
	timer_init(&TCD0,  TC_OVFINTLVL_OFF_gc);
	timer_set(&TCD0, TC_CLKSEL_DIV1024_gc, 0xFFFF);

	led_init();
	button_init();

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

int drivers_self_test(uint8_t interactive) //{{{
{
	if (interactive)
	{
		if (led_self_test())
			return 1;
		if (button_self_test())
			return 2;
	}

	if (pot_self_test())
		return 3;
	if (sram_self_test())
		return 4;
	if (sd_self_test())
		return 5;
	return 0;
} //}}}
