#include "common.h"

#include "error.h"
#include "blink.h"
#include "control.h"
#include "store.h"
#include "interrupt.h"
#include "samples.h"
#include "drivers.h"
#include "drivers/gpio.h"

int main() //{{{
{
	printf("main: clock_set_crystal()\r\n");
	clock_set_crystal();
	printf("main: interrupt_init()\r\n");
	interrupt_init();
	printf("main: drivers_init()\r\n");
	drivers_init();

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

			if (g_control_mode == CONTROL_MODE_STREAM && g_control_read_ready)
			{
				samples_uart_write(g_adca0, g_adcb0, len);
				g_control_read_ready = 0;

				interrupt_clear(INTERRUPT_DMA_CH0);
				interrupt_clear(INTERRUPT_DMA_CH2);

#ifdef GPIO_SAMPLE_WRITE_DONE
				gpio_toggle(&PORTE, (1<<GPIO_SAMPLE_WRITE_DONE));
#endif
			}
			if (g_control_mode == CONTROL_MODE_STORE)
			{
				samples_store_write(g_adca0, g_adcb0);

				interrupt_clear(INTERRUPT_DMA_CH0);
				interrupt_clear(INTERRUPT_DMA_CH2);
			}
		}
		
		uint16_t len = SAMPLES_LEN;

		// other ADCA and ADCB DMA channels (double buffered)
		if (interrupt_is_set(INTERRUPT_DMA_CH1) && interrupt_is_set(INTERRUPT_DMA_CH3))
		{

			// calibration finished, setup normal measurement operation
			if (!g_control_calibrated)
			{
				// voltage measurment
				ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN1_gc | ADC_CH_MUXNEG_GND_MODE3_gc; 
 				// current measurement
				mux_select(MUX_R);

				g_control_calibrated = 1;
			}

			if (g_control_mode == CONTROL_MODE_STREAM && g_control_read_ready)
			{
				samples_uart_write(g_adca1, g_adcb1, len);
				g_control_read_ready = 0;

				interrupt_clear(INTERRUPT_DMA_CH1);
				interrupt_clear(INTERRUPT_DMA_CH3);

#ifdef GPIO_SAMPLE_WRITE_DONE
				gpio_toggle(&PORTE, (1<<GPIO_SAMPLE_WRITE_DONE));
#endif
			}
			if (g_control_mode == CONTROL_MODE_STORE)
			{
				samples_store_write(g_adca1, g_adcb1);

				interrupt_clear(INTERRUPT_DMA_CH1);
				interrupt_clear(INTERRUPT_DMA_CH3);
			}

#ifdef GPIO_TRIGGER
			static int triggered = 0;
			if (!triggered)
			{
				dma_stop();
				dma_init(g_adca0, g_adca1, g_adcb0, g_adcb1, SAMPLES_LEN);
				while (!gpio_read(&PORTE, GPIO_TRIGGER));
				dma_start();
				triggered = 1;
			}
#endif
		}

		// need to read a file
		if (g_control_mode == CONTROL_MODE_READ_FILE && g_control_read_ready)
		{
			if (samples_store_read_next(g_adca0, g_adcb0) > 0)
				samples_uart_write(g_adca0, g_adcb0, len);
			else
				samples_uart_write(NULL, NULL, 0);
			g_control_read_ready = 0;
		}
	}

	return 0;
} //}}}
