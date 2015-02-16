#include "common.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "error.h"
#include "blink.h"
#include "control.h"
#include "store.h"
#include "interrupt.h"
#include "usbpower.h"
#include "samples.h"

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
	//usbpower_init();

	// sample timer
	EVSYS.CH0MUX = EVSYS_CHMUX_TCD0_OVF_gc; // event channel 0 will fire when TCD0 overflows
	timer_init(&TCD0,  TC_OVFINTLVL_OFF_gc);
	timer_set(&TCD0, TC_CLKSEL_DIV1024_gc, 0xFFFF);

	// GPIO for synchronization
	interrupt_disable();
	PORTE.DIR = 0;
	PORTE.PIN7CTRL = PORT_ISC_RISING_gc;
	PORTE.INT0MASK = PIN7_bm;
	PORTE.INTCTRL = PORT_INT0LVL_MED_gc;
	interrupt_enable();
} //}}}

#define GPIO_DISPLAY_DURATION 250 //ms
static int gpio_display = 0;
static volatile uint8_t *gpio_v_s;
static int gpio_mark_first_later = 0;

ISR(PORTE_INT0_vect)
{
	interrupt_set(INTERRUPT_GPIO);
	PORTE.INT0MASK &= ~PIN7_bm;
}

static void gpio_mark_recent_sample()
{
	uint16_t min_s = 0;
	uint16_t max_s = (SAMPLES_LEN / 2) - 1;
	uint16_t mid_s;
	uint16_t mid_b;

	// If the array is empty, then mark the first sample later once
	// the DMA for it occurs. This will be handled in the main loop.
	if (gpio_v_s[0] == 0) {
		gpio_mark_first_later = 1;
		return;
	}

	// Binary search to get close to the DMA boundary
	while ((max_s - min_s) > 5) {
		mid_s = min_s + ((max_s - min_s) / 2);
		mid_b = mid_s << 1;

		uint8_t cur_value = gpio_v_s[mid_b];
		if(cur_value != 0) {
			min_s = mid_s + 1; // DMA acting on later data
		}	else {
			max_s = mid_s; // DMA acting on earlier data
		}
	}

	// Linear search from the minimum to find exact DMA boundary
	for(mid_b = (min_s << 1); mid_b < SAMPLES_LEN; mid_b += 2) {
		if (gpio_v_s[mid_b] == 0) {
			gpio_v_s[mid_b - 1] |= 0x80;
			return;
		}
	}
	gpio_v_s[SAMPLES_LEN - 1] |= 0x80;
}


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
	/*
	if (store_init() >= 0)
	{
		printf("main: store init successful\r\n");
		store_run_commands();
	}
	*/

	// main loop for interrupt bottom halves 
	while (1) 
	{
		// turn off the CPU between interrupts 
		set_sleep_mode(SLEEP_SMODE_IDLE_gc); 
		sleep_enable();
		sleep_cpu();
		sleep_disable();

		// GPIO handling when DMA interrupts occur
		// 	- Marking first sample of previous buffer if required
		// 	- Changing the sample array the GPIO handler looks at
		if (interrupt_is_set(INTERRUPT_DMA_CH0) && interrupt_is_set(INTERRUPT_DMA_CH2))
		{
			if (gpio_mark_first_later == 1) {
				gpio_v_s[1] |= 0x80;
				gpio_mark_first_later = 0;
			}
			gpio_v_s = (volatile uint8_t*)g_adca1;
		} else if (interrupt_is_set(INTERRUPT_DMA_CH1) && interrupt_is_set(INTERRUPT_DMA_CH3))
		{
			if (gpio_mark_first_later == 1) {
				gpio_v_s[1] |= 0x80;
				gpio_mark_first_later = 0;
			}
			gpio_v_s = (volatile uint8_t*)g_adca0;
		}

		// GPIO handling when GPIO interrupt occurs
		// 	- Mark the most recent sample in the current DMA buffer
		// 	- Enable the red LED for a short time
		if (interrupt_is_set(INTERRUPT_GPIO)) {
			interrupt_clear(INTERRUPT_GPIO);
			PORTE.INT0MASK |= PIN7_bm;

			gpio_mark_recent_sample();
			led_on(LED_RED_bm);
			gpio_display = 0;
		}

		// ADCA and ADCB DMA channels
		if (interrupt_is_set(INTERRUPT_DMA_CH0) && interrupt_is_set(INTERRUPT_DMA_CH2))
		{
			if (g_control_mode == CONTROL_MODE_STREAM)
				samples_uart_write(g_adca0, g_adcb0, SAMPLES_LEN);
			if (g_control_mode == CONTROL_MODE_STORE)
				samples_store_write(g_adca0, g_adcb0);
			interrupt_clear(INTERRUPT_DMA_CH0);
			interrupt_clear(INTERRUPT_DMA_CH2);
		}
		
		// other ADCA and ADCB DMA channels (double buffered)
		if (interrupt_is_set(INTERRUPT_DMA_CH1) && interrupt_is_set(INTERRUPT_DMA_CH3))
		{
			if (g_control_mode == CONTROL_MODE_STREAM)
				samples_uart_write(g_adca1, g_adcb1, SAMPLES_LEN);
			if (g_control_mode == CONTROL_MODE_STORE)
				samples_store_write(g_adca1, g_adcb1);
			interrupt_clear(INTERRUPT_DMA_CH1);
			interrupt_clear(INTERRUPT_DMA_CH3);
		}

		// UART receive
		if (interrupt_is_set(INTERRUPT_UART_RX))
		{
			interrupt_clear(INTERRUPT_UART_RX);
			control_got_uart_bytes();
		}

		// general ms timer
		if (interrupt_is_set(INTERRUPT_TIMER_MS))
		{
			interrupt_clear(INTERRUPT_TIMER_MS);
			blink_ms_timer_update();

			if (gpio_display < GPIO_DISPLAY_DURATION) {
				if (++gpio_display == GPIO_DISPLAY_DURATION) {
					led_off(LED_RED_bm);
				}
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
