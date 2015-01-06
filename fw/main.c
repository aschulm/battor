#include "common.h"

#include "error.h"
#include "blink.h"
#include "control.h"
#include "store.h"
#include "interrupt.h"
#include "usbpower.h"
#include "samples.h"

uint32_t total = 0;

void init_devices() //{{{
{
	// order of init is very important
	printf("init_devices: clock_set_crystal()\r\n");
	clock_set_crystal();

	// ms timer for blinking led and other real time tasks
	printf("init_devices: timer_init()\r\n");
	timer_init(&TCC0, TC_OVFINTLVL_LO_gc);
	printf("init_devices: timer_set()\r\n");
	timer_set(&TCC0, TC_CLKSEL_DIV64_gc, 499);

	printf("init_devices: led_init()\r\n");
	led_init();
	printf("init_devices: mux_init()\r\n");
	mux_init();
	printf("init_devices: uart_init()\r\n");
	uart_init();
	printf("init_devices: pot_init()\r\n");
	pot_init();
	printf("init_devices: adc_init(ADCA)\r\n");
	adc_init(&ADCA); // voltage ADC
	printf("init_devices: adc_init(ADCB)\r\n");
	adc_init(&ADCB); // current ADC
	printf("init_devices: usb power init\r\n");
	usbpower_init();

	// sample timer
	timer_init(&TCD0,  TC_OVFINTLVL_OFF_gc);
	timer_set(&TCD0, TC_CLKSEL_DIV1024_gc, 0xFFFF);
} //}}}

int main() //{{{
{
	printf("main: interrupt_init()\r\n");
	interrupt_init();
	printf("main: init_devices()\r\n");
	init_devices();

	// setup an LED to blink while running, start with yellow to indicate not ready yet 
	blink_init(1000, LED_YELLOW_bm); 

	// try to initalize storage
	g_control_mode = 0;
	if (store_init() >= 0)
	{
		printf("main: store init successful\r\n");
		store_run_commands();
	}

	// main loop for interrupt bottom halves 
	while (1) 
	{
		// turn off the CPU between interrupts 
		set_sleep_mode(SLEEP_SMODE_IDLE_gc); 
		sleep_enable();
		sleep_cpu();
		sleep_disable();

		// general ms timer
		if (interrupt_is_set(INTERRUPT_TIMER_MS))
		{
			blink_ms_timer_update();
			interrupt_clear(INTERRUPT_TIMER_MS);
		}

		// UART receive
		if (interrupt_is_set(INTERRUPT_UART_RX))
		{
			control_got_uart_bytes();
			interrupt_clear(INTERRUPT_UART_RX);
		}

		// ADCA and ADCB DMA channels
		if (interrupt_is_set(INTERRUPT_DMA_CH0) && interrupt_is_set(INTERRUPT_DMA_CH2))
		{
			uint16_t len = SAMPLES_LEN;

			len = samples_ovsample(g_adca0, g_adcb0, len);

			if (g_control_mode == CONTROL_MODE_STREAM)
				samples_uart_write(g_adca0, g_adcb0, len*sizeof(int16_t));
			if (g_control_mode == CONTROL_MODE_STORE)
				samples_store_write(g_adca0, g_adcb0);
			interrupt_clear(INTERRUPT_DMA_CH0);
			interrupt_clear(INTERRUPT_DMA_CH2);
		}
		
		// other ADCA and ADCB DMA channels (double buffered)
		if (interrupt_is_set(INTERRUPT_DMA_CH1) && interrupt_is_set(INTERRUPT_DMA_CH3))
		{
			uint16_t len = SAMPLES_LEN;

			// calibration finished, setup normal measurement operation
			if (!g_control_calibrated)
			{
				ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN1_gc | ADC_CH_MUXNEG_GND_MODE3_gc; // voltage measurment
				mux_select(MUX_R); // current measurement
				g_control_calibrated = 1;

				// let the ADC settle
				dma_stop();
				timer_sleep_ms(10);
				dma_start();
			}

			len = samples_ovsample(g_adca1, g_adcb1, len);

			if (g_control_mode == CONTROL_MODE_STREAM)
				samples_uart_write(g_adca1, g_adcb1, len*sizeof(int16_t));
			if (g_control_mode == CONTROL_MODE_STORE)
				samples_store_write(g_adca1, g_adcb1);
			interrupt_clear(INTERRUPT_DMA_CH1);
			interrupt_clear(INTERRUPT_DMA_CH3);
		}

		// need to read a file
		if (g_control_mode == CONTROL_MODE_READ_FILE)
		{
			g_control_mode = CONTROL_MODE_IDLE;
			samples_store_read(g_samples_read_file);
		}
	}

	return 0;
} //}}}
