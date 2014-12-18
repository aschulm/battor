#include "common.h"

#include "error.h"
#include "control.h"
#include "blink.h"
#include "samples.h"
#include "usbpower.h"
#include "store.h"

static control_message message;
static int message_b_idx = 0;
uint8_t g_control_mode;
uint8_t g_control_calibrated;

void control_got_uart_bytes() //{{{
{
	uint8_t i;
	uint8_t recv_len = 1;
	uint8_t* message_b = (uint8_t*)&message;

	while (recv_len > 0) // stop when there is no uart data left
	{
		// need more bytes
		if (message_b_idx < sizeof(control_message))
		{
			recv_len = uart_rx_bytes((message_b + message_b_idx), sizeof(control_message) - message_b_idx);
			message_b_idx += recv_len;
		}

		// have enough bytes
		if (message_b_idx == sizeof(control_message))
		{
			if (message.delim[0] == CONTROL_DELIM0 && 
				message.delim[1] == CONTROL_DELIM1)
			{
				control_run_message(&message);
				uart_tx_byte(CONTROL_ACK); // send an ack
				message_b_idx = 0; // done with the message
			}
			else
			{
				// chuck everything till what looks like the start of a delimiter
				while (message.delim[0] != CONTROL_DELIM0 && message_b_idx > 0)
				{
					// shift over all bytes by one, and shrink the message
					message_b_idx--;
					for (i = 0; i < message_b_idx; i++)
						message_b[i] = message_b[i+1];
				}
			}
		}
	}
} //}}}

void control_run_message(control_message* m) //{{{
{
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
			case CONTROL_TYPE_AMPPOT_SET:
				pot_wiperpos_set(POT_AMP_CS_PIN_gm, m->value1);
				pos = pot_wiperpos_get(POT_AMP_CS_PIN_gm);
				if (m->value1 != pos)
					halt();
			break;
			case CONTROL_TYPE_FILPOT_SET:
				pot_wiperpos_set(POT_FIL_CS_PIN_gm, m->value1);
				pos = pot_wiperpos_get(POT_FIL_CS_PIN_gm);
				if (m->value1 != pos)
					halt();
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
				g_control_calibrated = 0;
				ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN7_gc | ADC_CH_MUXNEG_PIN7_gc; // voltage to gnd
				mux_select(MUX_GND); // current to gnd 
				dma_init(g_adca0, g_adca1, g_adcb0, g_adcb1, SAMPLES_LEN);
				dma_start(); // start getting samples from the ADCs
			break;
			case CONTROL_TYPE_START_SAMPLING_SD:
				g_control_mode = CONTROL_MODE_STORE;
				blink_set_led(LED_GREEN_bm | LED_YELLOW_bm);
				blink_set_strobe_count(store_write_open());
				// setup calibration
				g_control_calibrated = 0;
				ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN7_gc | ADC_CH_MUXNEG_PIN7_gc; // voltage to gnd
				mux_select(MUX_GND); // current to gnd 
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
		}
	}
} //}}}
