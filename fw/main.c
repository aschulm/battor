#include "common.h"

#include "error.h"
#include "blink.h"
#include "control.h"
#include "store.h"
#include "interrupt.h"
#include "samples.h"
#include "drivers.h"

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
			}
			if (g_control_mode == CONTROL_MODE_STORE)
			{
				samples_store_write(g_adca0, g_adcb0);

				interrupt_clear(INTERRUPT_DMA_CH0);
				interrupt_clear(INTERRUPT_DMA_CH2);
			}
		}
		
		// other ADCA and ADCB DMA channels (double buffered)
		if (interrupt_is_set(INTERRUPT_DMA_CH1) && interrupt_is_set(INTERRUPT_DMA_CH3))
		{
			uint16_t len = SAMPLES_LEN;

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
			}
			if (g_control_mode == CONTROL_MODE_STORE)
			{
				samples_store_write(g_adca1, g_adcb1);

				interrupt_clear(INTERRUPT_DMA_CH1);
				interrupt_clear(INTERRUPT_DMA_CH3);
			}
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
