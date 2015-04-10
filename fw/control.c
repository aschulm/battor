#include "common.h"

#include "error.h"
#include "control.h"
#include "blink.h"
#include "samples.h"
#include "usbpower.h"
#include "store.h"

static control_message message;
uint8_t g_control_mode;
uint8_t g_control_calibrated;
uint8_t g_control_read_ready;

void control_got_uart_bytes() //{{{
{
	uint8_t recv_len;
	uint8_t type;
	int8_t ret = 0;

	recv_len = uart_rx_bytes(&type, (uint8_t*)&message, sizeof(message));

	if (recv_len == sizeof(control_message))
	{
		ret = control_run_message(&message);

		if (ret >= 0)
		{
			// send ack
			uart_tx_start(UART_TYPE_CONTROL_ACK);
			uart_tx_bytes(&message.type, sizeof(message.type)); 
			uart_tx_bytes(&ret, 1);
			uart_tx_end();
		}
	}
} //}}}

int8_t control_run_message(control_message* m) //{{{
{
	uint8_t ret = 0;

	printf("control: type %d\r\n", m->type);
	// record the message if we are in record mode, else run it
	if (g_control_mode == CONTROL_MODE_REC_CONTROL &&
			m->type != CONTROL_TYPE_START_REC_CONTROL &&
			m->type != CONTROL_TYPE_END_REC_CONTROL)
	{
		store_rec_control(m);
	}
	else
	{
		uint16_t pos;
		switch (m->type)
		{
			case CONTROL_TYPE_INIT:
				g_control_mode = CONTROL_MODE_IDLE;
				ret = g_error_last;
				g_error_last = 0;
				dma_stop(); // stop getting samples from the ADCs
			break;
			case CONTROL_TYPE_AMPPOT_SET:
				pot_wiperpos_set(POT_AMP_CS_PIN_gm, m->value1);
				pos = pot_wiperpos_get(POT_AMP_CS_PIN_gm);
				if (m->value1 != pos)
					halt(ERROR_AMPPOT_SET_FAILED);
			break;
			case CONTROL_TYPE_FILPOT_SET:
				pot_wiperpos_set(POT_FIL_CS_PIN_gm, m->value1);
				pos = pot_wiperpos_get(POT_FIL_CS_PIN_gm);
				if (m->value1 != pos)
					halt(ERROR_FILPOT_SET_FAILED);
			break;
			case CONTROL_TYPE_SAMPLE_TIMER_SET:
				timer_set(&TCD0, m->value1, m->value2);	 // prescaler, period
			break;
			case CONTROL_TYPE_OVERSAMPLING_SET:
				g_samples_ovsamp_bits = m->value1;
			break;
			case CONTROL_TYPE_START_SAMPLING_UART:
				g_control_mode = CONTROL_MODE_STREAM;
				g_samples_uart_seqnum = 0;
				blink_set_led(LED_GREEN_bm);

				// setup calibration mode
				g_control_calibrated = 0;
				// current measurment input to gnd
				mux_select(MUX_GND); 
				// voltage to voltage to measure offset, for some reason it's better to do v to v than gnd to gnd
				ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN7_gc | ADC_CH_MUXNEG_GND_MODE3_gc; 
				// wait for things to settle
				timer_sleep_ms(100);

				// start dma
				dma_init(g_adca0, g_adca1, g_adcb0, g_adcb1, SAMPLES_LEN);
				dma_start(); // start getting samples from the ADCs
			break;
			case CONTROL_TYPE_START_SAMPLING_SD:
				g_control_mode = CONTROL_MODE_STORE;
				blink_set_led(LED_GREEN_bm | LED_YELLOW_bm);
				blink_set_strobe_count(store_write_open());

				// setup calibration
				g_control_calibrated = 0;
				// current measurement input to gnd
				mux_select(MUX_GND);
				// voltage to voltage to measure offset, for some reason it's better to do v to v than gnd to gnd
				ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN7_gc | ADC_CH_MUXNEG_GND_MODE3_gc;
				// wait for things to settle
				timer_sleep_ms(100);

				// start dma
				dma_init(g_adca0, g_adca1, g_adcb0, g_adcb1, SAMPLES_LEN);
				dma_start(); // start getting samples from the ADCs
			break;
			case CONTROL_TYPE_START_REC_CONTROL:
				g_control_mode = CONTROL_MODE_REC_CONTROL;
				store_start_rec_control(m->value1);
			break;
			case CONTROL_TYPE_END_REC_CONTROL:
				g_control_mode = CONTROL_MODE_IDLE;
				store_end_rec_control();
			break;
			case CONTROL_TYPE_READ_FILE:
				g_control_mode = CONTROL_MODE_READ_FILE;
				g_samples_uart_seqnum = 0;
				g_samples_read_file = m->value1;
				dma_stop(); // will get samples from the file
			break;
			case CONTROL_TYPE_USB_POWER_SET:
				usbpower_set(m->value1);
			break;
			case CONTROL_TYPE_READ_READY:
				g_control_read_ready = 1;
				ret = -1; // don't send ack
			break;
		}
	}
	return ret;
} //}}}
