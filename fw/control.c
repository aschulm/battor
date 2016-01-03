#include "common.h"

#include "error.h"
#include "control.h"
#include "blink.h"
#include "samples.h"
#include "drivers.h"
#include "ringbuf.h"
#include "params.h"

static control_message message;
uint8_t g_control_mode = 0;
uint8_t g_control_gain = 0;

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

			// flush rx buffer to avoid running control messages
			// that came in during the execution of the last one
			// (e.g., another read ready)
			uart_rx_flush();
		}
	}
} //}}}

int8_t control_run_message(control_message* m) //{{{
{
	int8_t ret = 0;

	printf("control: type %d\n", m->type);
	uint8_t buf[100];
	switch (m->type)
	{
		case CONTROL_TYPE_INIT:
			// reset if already run
			if (g_control_mode != 0)
				reset();

			g_control_mode = CONTROL_MODE_IDLE;

			ret = g_error_last;
			// an error may not have been set yet, always return something
			if (ret < 0)
				ret = 0;

			// clear out last error
			g_error_last = 0;
		break;
		case CONTROL_TYPE_GAIN_SET:
			g_control_gain = m->value1;
		break;
		case CONTROL_TYPE_START_SAMPLING_UART:
			g_control_mode = CONTROL_MODE_STREAM;
			blink_set_led(LED_GREEN_bm);
			samples_start();
		break;
		case CONTROL_TYPE_START_SAMPLING_SD:
			g_control_mode = CONTROL_MODE_STORE;
			blink_set_led(LED_RED_bm);
			samples_start();

		break;
		//case CONTROL_TYPE_READ_FILE:
		//	g_control_mode = CONTROL_MODE_READ_FILE;
		//	g_samples_uart_seqnum = 0;
		//	store_read_open(m->value1);
		//	dma_stop(); // will get samples from the file
		//break;
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

			// step 3. test the filesystem 
			if ((ret = fs_self_test()) > 0)
				break;
		break;
	}

	return ret;
} //}}}

