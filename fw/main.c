#include "common.h"

#include "error.h"
#include "blink.h"
#include "control.h"
#include "interrupt.h"
#include "samples.h"
#include "store.h"
#include "usbpower.h"

uint8_t g_adca0[SAMPLES_LEN], g_adca1[SAMPLES_LEN], g_adcb0[SAMPLES_LEN], g_adcb1[SAMPLES_LEN];

void init_devices() //{{{
{
	// order of init is very important
	printf("init_devices: clock_set_crystal()\r\n");
	clock_set_crystal();
	// ms timer for blinking led and other real time tasks
	printf("init_devices: timer_init()\r\n");
	timer_init(&TCC0, TC_OVFINTLVL_LO_gc);
	printf("init_devices: timer_set()\r\n");
	timer_set(&TCC0, TC_CLKSEL_DIV64_gc, 250);
	printf("init_devices: led_init()\r\n");
	led_init();
	printf("init_devices: uart_init()\r\n");
	uart_init();
	printf("init_devices: pot_init()\r\n");
	pot_init();
	printf("init_devices: adc_init(ADCA)\r\n");
	adc_init(&ADCA); // voltage ADC
	printf("init_devices: adc_init(ADCB)\r\n");
	adc_init(&ADCB); // current ADC
	printf("init_devices: dma_init()\r\n");
	dma_init(g_adca0, g_adca1, g_adcb0, g_adcb1, SAMPLES_LEN);
	printf("init_devices: usb power init\r\n");
	usbpower_init();

	// sample timer
	EVSYS.CH0MUX = EVSYS_CHMUX_TCD0_OVF_gc; // event channel 0 will fire when TCD0 overflows
	timer_init(&TCD0,  TC_OVFINTLVL_OFF_gc);
	timer_set(&TCD0, TC_CLKSEL_DIV1024_gc, 0xFFFF);
	
} //}}}

int main() //{{{
{
	printf("main: interrupt_init()\r\n");
	interrupt_init();
	printf("main: init_devices()\r\n");
	init_devices();

	g_control_mode = 0; // not sampling yet
	printf("main: blink_init()\r\n");
	blink_init(750, LED_YELLOW_bm); // setup an LED to blink while running, start with yellow to indicate not ready yet 

	// try to initiate storage 
	//if (store_init())
	//{
	//}

	// main loop for interrupt bottom halves 
	while (1) 
	{
		// turn off the CPU between interrupts 
		printf("main loop: sleeps()\r\n");
		set_sleep_mode(SLEEP_SMODE_IDLE_gc); 
		sleep_enable();
		sleep_cpu();
		sleep_disable();

		// general ms timer
		if (interrupt_is_set(INTERRUPT_TIMER_MS))
		{
			interrupt_clear(INTERRUPT_TIMER_MS);
			printf("main: blink_ms_time_update\r\n");
			blink_ms_timer_update();
		}

		// UART receive
		if (interrupt_is_set(INTERRUPT_UART_RX))
		{
			interrupt_clear(INTERRUPT_UART_RX);
			control_got_uart_bytes();
		}

		// ADCA and ADCB DMA channels
		if (interrupt_is_set(INTERRUPT_DMA_CH0) && interrupt_is_set(INTERRUPT_DMA_CH2))
		{
			if (g_control_mode == CONTROL_MODE_STREAM)
				samples_uart_write(g_adca0, g_adcb0);
			// TODO write to SD card
			//if (g_control_mode == CONTROL_MODE_STORE)
				//samples_sd_write(g_adca0, g_adcb0, &samples_file);
			interrupt_clear(INTERRUPT_DMA_CH0);
			interrupt_clear(INTERRUPT_DMA_CH2);
		}
		
		// other ADCA and ADCB DMA channels (double buffered)
		if (interrupt_is_set(INTERRUPT_DMA_CH1) && interrupt_is_set(INTERRUPT_DMA_CH3))
		{
			if (g_control_mode == CONTROL_MODE_STREAM)
				samples_uart_write(g_adca1, g_adcb1);
			// TODO write to SD card
			//if (g_control_mode == CONTROL_MODE_STORE)
			//	samples_sd_write(g_adca1, g_adcb1, &samples_file);
			interrupt_clear(INTERRUPT_DMA_CH1);
			interrupt_clear(INTERRUPT_DMA_CH3);
		}
	}
	return 0;
} //}}}
