#include "common.h"

#include "error.h"
#include "control.h"
#include "blink.h"
#include "samples.h"
#include "store.h"
#include "drivers.h"
#include "ringbuf.h"

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

	printf("control: type %d\n", m->type);
	// record the message if we are in record mode, else run it
	if (g_control_mode == CONTROL_MODE_REC_CONTROL &&
			m->type != CONTROL_TYPE_START_REC_CONTROL &&
			m->type != CONTROL_TYPE_END_REC_CONTROL)
	{
		store_rec_control(m);
	}
	else
	{
		int8_t filenum;
		uint16_t pos;
		uint8_t buf[100];
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
			case CONTROL_TYPE_START_SAMPLING_UART:
				g_control_mode = CONTROL_MODE_STREAM;
				g_samples_uart_seqnum = 0;
				blink_set_led(LED_GREEN_bm);

				// setup calibration mode
				g_control_calibrated = 0;
				// current measurement input to gnd
				mux_select(MUX_GND);
				// voltage measurement input to gnd
				ADCB.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN7_gc | ADC_CH_MUXNEG_GND_MODE3_gc;
				// wait for things to settle
				timer_sleep_ms(10);

				// init dma, but do not start until control read ready message
				dma_init(g_adcb0, g_adcb1, SAMPLES_LEN*sizeof(sample));
			break;
			case CONTROL_TYPE_START_SAMPLING_SD:
				filenum = store_write_open();

				// max files reached, return to idle
				if (filenum < 0)
				{
					g_control_mode = CONTROL_MODE_IDLE;
					ret = -1;
				}
				else
				{
					g_control_mode = CONTROL_MODE_STORE;
					blink_set_led(LED_RED_bm);
					blink_set_strobe_count(filenum);

					// setup calibration
					g_control_calibrated = 0;
					// current measurement input to gnd
					mux_select(MUX_GND);
					// voltage measurement input to gnd
					ADCB.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN7_gc | ADC_CH_MUXNEG_GND_MODE3_gc;
					// wait for things to settle
					timer_sleep_ms(10);

					// start dma
					dma_init(g_adcb0, g_adcb1, SAMPLES_LEN*sizeof(sample));
					dma_start(); // start getting samples from the ADCs
				}
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
				store_read_open(m->value1);
				dma_stop(); // will get samples from the file
			break;
			case CONTROL_TYPE_READ_READY:
				// first frame, start getting samples from the ADCs
				if (g_samples_uart_seqnum == 0)
					dma_start(); 

				g_control_read_ready = 1;
				ret = -1; // don't send ack
			break;
			case CONTROL_TYPE_RESET:
				reset();
			break;
			case CONTROL_TYPE_READ_EEPROM:
				EEPROM_read_block(0x0000, buf, m->value1);	
				uart_tx_start(UART_TYPE_CONTROL_ACK);
				uart_tx_bytes(buf, m->value1);
				uart_tx_end();
				ret = -1;
			break;
			case CONTROL_TYPE_SELF_TEST:
				// step 1. test the sram
				printf("====== self test started ======\n");
				if ((ret = drivers_self_test()) > 0)
					break;

				// step 2. test the ringbuf
				if ((ret = ringbuf_self_test()) > 0)
					break;
			break;
		}
	}
	return ret;
} //}}}

