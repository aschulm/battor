#include "common.h"

#include "error.h"
#include "blink.h"
#include "control.h"
#include "interrupt.h"
#include "samples.h"
#include "drivers.h"
#include "params.h"

int main() //{{{
{
	clock_set_crystal();
	interrupt_init();
	drivers_init();
	params_init();

	// setup an LED to blink while running, start with yellow to indicate not ready yet 
	blink_init(1000, LED_YELLOW_bm); 

	g_control_mode = 0;

	// main loop for interrupt bottom halves 
	while (1) 
	{
#ifdef GPIO_MAIN_LOOP
			gpio_toggle(&PORTE, (1<<GPIO_MAIN_LOOP));
#endif

		// turn off the CPU between interrupts 
		set_sleep_mode(SLEEP_SMODE_IDLE_gc); 
		sleep_enable();
		sleep_cpu();
		sleep_disable();

		/*
		 * The order of these lower half handlers reflects their priority
		 */

		// general ms timer
		if (interrupt_is_set(INTERRUPT_TIMER_MS))
		{
			blink_ms_timer_update();
			interrupt_clear(INTERRUPT_TIMER_MS);
		}

		// ADCB DMA (channel 2)
		if (interrupt_is_set(INTERRUPT_DMA_CH2))
		{
			// put samples on FIFO
			samples_ringbuf_write(g_adcb0);
			interrupt_clear(INTERRUPT_DMA_CH2);
		}

		// other ADCB DMA (channel 3, double buffered)
		if (interrupt_is_set(INTERRUPT_DMA_CH3))
		{
			// put samples on FIFO
			samples_ringbuf_write(g_adcb1);
			interrupt_clear(INTERRUPT_DMA_CH3);
		}

		if (g_control_mode == CONTROL_MODE_STREAM)
		{
			samples_uart_write(0);

#ifdef GPIO_SAMPLE_WRITE_DONE
			gpio_toggle(&PORTE, (1<<GPIO_SAMPLE_WRITE_DONE));
#endif
		}

		if (g_control_mode == CONTROL_MODE_STORE)
		{
			samples_store_write();
		}

		// UART receive
		if (interrupt_is_set(INTERRUPT_UART_RX))
		{
			control_got_uart_bytes();
			interrupt_clear(INTERRUPT_UART_RX);
		}
	}

	return 0;
} //}}}
