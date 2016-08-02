#include "common.h"

#include "error.h"
#include "control.h"
#include "blink.h"
#include "samples.h"
#include "drivers.h"
#include "ringbuf.h"
#include "fs.h"
#include "params.h"

static control_message message;
static uint8_t inited = 0;
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
			uart_tx_start_prepare(UART_TYPE_CONTROL_ACK);
			uart_tx_bytes_prepare(&message.type, sizeof(message.type)); 
			uart_tx_bytes_prepare(&ret, 1);
			uart_tx_end_prepare();
			uart_tx();

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
	uint32_t u32;
	switch (m->type)
	{
		case CONTROL_TYPE_INIT:
			// reset if already run
			if (inited)
				reset();

			ret = g_error_last;
			// an error may not have been set yet, always return something
			if (ret < 0)
				ret = 0;

			// clear out last error
			g_error_last = 0;

			inited = 1;
		break;
		case CONTROL_TYPE_GAIN_SET:
			g_control_gain = m->value1;
		break;
		case CONTROL_TYPE_START_SAMPLING_UART:
			g_control_mode = CONTROL_MODE_STREAM;
			blink_set_led(LED_GREEN_bm);
			blink_set_strobe_count(1);
			samples_start();
		break;
		case CONTROL_TYPE_START_SAMPLING_SD:
			// indicate to the user that the battor is in usb control mode
			led_off(LED_GREEN_bm);

			// format into usb mode if in portable mode
			if (g_control_mode == CONTROL_MODE_PORT_IDLE ||
				g_control_mode == CONTROL_MODE_PORT_STORE)
				if (fs_format(0) < 0)
					halt(ERROR_FS_FORMAT_FAIL);

			g_control_mode = CONTROL_MODE_USB_STORE;
			blink_set_led(LED_RED_bm);
			blink_set_strobe_count(1);
			samples_start();
		break;
		case CONTROL_TYPE_READ_SD_UART:
			led_on(LED_RED_bm);

			// stopping portable store so indicate the next file to the user
			if (g_control_mode == CONTROL_MODE_PORT_STORE)
				blink_set_strobe_count(g_fs_file_seq + 1);

			samples_stop();

			// return to idle mode depending on which mode it is in
			// if it is already idle then do not change the mode
			if (g_control_mode == CONTROL_MODE_USB_STORE)
				g_control_mode = CONTROL_MODE_USB_IDLE;
			if (g_control_mode == CONTROL_MODE_PORT_STORE)
				g_control_mode = CONTROL_MODE_PORT_IDLE;

			samples_store_read_uart_write(m->value1);
			led_off(LED_RED_bm);
			blink_set_led(LED_YELLOW_bm);
		break;
		case CONTROL_TYPE_RESET:
			reset();
		break;
		case CONTROL_TYPE_READ_EEPROM:
			EEPROM_read_block(0x0000, buf, m->value1);	
			uart_tx_start_prepare(UART_TYPE_CONTROL_ACK);
			uart_tx_bytes_prepare(buf, m->value1);
			uart_tx_end_prepare();
			uart_tx_dma();
			ret = -1;
		break;
		case CONTROL_TYPE_GET_SAMPLE_COUNT:
			// abort if UART is not already ready to prevent overflow
			if (!uart_tx_ready())
			{
				ret = -1;
				break;
			}

			u32 = uart_rx_sample_count();
			uart_tx_start_prepare(UART_TYPE_CONTROL_ACK);
			uart_tx_bytes_prepare(&u32, sizeof(u32));
			uart_tx_end_prepare();

			uart_tx_dma();
			while (!dma_uart_tx_ready());

			// toggle CTS to force FTDI chip to flush the buffer
			uart_set_rts(0);
			uart_set_rts(1);

			ret = -1;
		break;
		case CONTROL_TYPE_GET_GIT_HASH:
			uart_tx_start_prepare(UART_TYPE_CONTROL_ACK);
			uart_tx_bytes_prepare(GIT_HASH, strlen(GIT_HASH));
			uart_tx_end_prepare();
			uart_tx_dma();
			ret = -1;
		break;
		case CONTROL_TYPE_GET_MODE_PORTABLE:
			ret = (g_control_mode == CONTROL_MODE_PORT_IDLE ||
				g_control_mode == CONTROL_MODE_PORT_STORE);
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

void control_button_press() //{{{
{
	switch(g_control_mode)
	{
		case CONTROL_MODE_PORT_IDLE:
			g_control_mode = CONTROL_MODE_PORT_STORE;
			blink_set_led(LED_RED_bm);
			samples_start();

			// indicate to the user the current open file seq
			blink_set_strobe_count(g_fs_file_seq);
		break;
		case CONTROL_MODE_PORT_STORE:
			// indicate to the user what the next open file seq is
			blink_set_led(LED_YELLOW_bm);
			blink_set_strobe_count(g_fs_file_seq + 1);

			samples_stop();
			led_off(LED_RED_bm);
			g_control_mode = CONTROL_MODE_PORT_IDLE;
		break;
	}
} //}}}

void control_button_hold() //{{{
{
	// stop storing current file
	if (g_control_mode == CONTROL_MODE_PORT_STORE ||
		g_control_mode == CONTROL_MODE_USB_STORE)
	{
			samples_stop();
			led_off(LED_RED_bm);
			blink_set_led(LED_YELLOW_bm);
			g_control_mode = CONTROL_MODE_PORT_IDLE;
	}

	// format SD card in portable mode
	if (fs_format(1) < 0)
		halt(ERROR_FS_FORMAT_FAIL);

	g_control_mode = CONTROL_MODE_PORT_IDLE;

	// indicate to user that SD card is at the first file and in portable mode
	blink_set_strobe_count(g_fs_file_seq);
	led_on(LED_GREEN_bm);
} //}}}
