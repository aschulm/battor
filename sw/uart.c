#include <libftdi1/ftdi.h>

#include "common.h"
#include "uart.h"
#include "params.h"
#include "samples.h"

static struct ftdi_context *ftdi;
static uint8_t uart_rx_buffer[UART_RX_BUFFER_LEN];
static uint8_t uart_tx_buffer[UART_RX_BUFFER_LEN];
static int uart_rx_buffer_write_idx = 0;
static int uart_rx_buffer_read_idx = 0;
static int uart_tx_buffer_idx = 0;

int uart_rx_byte(uint8_t* b) //{{{
{
	int i, attempt = 0;

	while (uart_rx_buffer_read_idx >= uart_rx_buffer_write_idx)
	{
		if (attempt++ >= UART_RX_ATTEMPTS)
		{
            vverb_printf("uart_rx_byte: timeout\n%s", "");
			return -1;
		}

		if ((uart_rx_buffer_write_idx = ftdi_read_data(ftdi, uart_rx_buffer, sizeof(uart_rx_buffer))) < 0)
		{
            vverb_printf("uart_rx_byte: ftdi_read_data error\n%s", "");
			return -1;
		}
		uart_rx_buffer_read_idx = 0;

		vverb_printf("uart_rx_byte: len:%d", uart_rx_buffer_write_idx);
		for (i = 0; i < uart_rx_buffer_write_idx; i++)
		{
			vverb_printf(" %.2X", uart_rx_buffer[i]);
		}
		vverb_printf("%s\n", "");
	}

	*b = uart_rx_buffer[uart_rx_buffer_read_idx++];
	return uart_rx_buffer_write_idx;
} //}}}

int uart_rx_bytes(uint8_t* type, void* b, uint16_t b_len) //{{{
{
	int i, ret;
    uint8_t b_b[UART_RX_BUFFER_LEN];
	uint8_t b_tmp;
	uint8_t start_found = 0;
	uint8_t end_found = 0;
	uint16_t bytes_read = 0;

uart_rx_bytes_start:
    start_found = end_found = bytes_read = 0;

	vverb_printf("uart_rx_bytes: start\n%s", "");

	while (!(start_found && end_found))
	{
		if (uart_rx_byte(&b_tmp) <= 0)
		{
			vverb_printf("uart_rx_bytes: failed to get byte\n%s", "");
			return -1;
		}

		if (b_tmp == UART_ESC_DELIM)
		{
			vverb_printf("uart_rx_bytes: got escape\n%s", "");
			if (uart_rx_byte(&b_tmp) <= 0)
			{
				vverb_printf("uart_rx_bytes: got escape -- failed to get next byte\n%s", "");
				return -1;
			}

			if (bytes_read < UART_RX_BUFFER_LEN)
			{
				vverb_printf("uart_rx_byte: got escape --- stored escaped byte 0x%X\n", b_tmp);
				b_b[bytes_read++] = b_tmp;
			}
		}
		else if (b_tmp == UART_START_DELIM)
		{
            if (start_found) {
                vverb_printf("uart_rx_byte: got two start delimiters\n%s", "");
                return -1;
            }
			start_found = 1;
			bytes_read = 0;

			if (uart_rx_byte(type) <= 0)
			{
				vverb_printf("uart_rx_byte: got start -- failed to get next byte\n%s", "");
				return -1;
			}
			vverb_printf("uart_rx_byte: got start type 0x%X\n", *type);
		}
		else if (b_tmp == UART_END_DELIM)
		{
            if (!start_found) {
                vverb_printf("uart_rx_byte: got end delimiter without start\n%s", "");
                return -1;
            }
			vverb_printf("uart_rx_byte: got end\n%s", "");
			end_found = 1;
		}
		else if (bytes_read < UART_RX_BUFFER_LEN)
		{
            if (!start_found) {
                vverb_printf("uart_rx_byte: got data before start 0x%X\n", b_tmp);
            } else {
                vverb_printf("uart_rx_byte: got byte 0x%X\n", b_tmp);
                b_b[bytes_read++] = b_tmp;
            }
		}
        /*else
        {
			vverb_printf("uart_rx_byte: nothing worked so done\n%s", "");
            return -1;
        }*/
	}

	// verbose
    if (*type == UART_TYPE_PRINT) {
        b_b[bytes_read] = '\0';
        printf("[DEBUG] %s", b_b);
        goto uart_rx_bytes_start;
    } else {
        verb_printf("uart_rx_bytes: recv%s", "");
        for (i = 0; i < bytes_read; i++)
        {
            verb_printf(" %.2X", b_b[i]);
        }
        verb_printf("%s\n", "");
    }

    memcpy(b, b_b, b_len); // Copy as many bytes as was asked for

	return bytes_read;
} //}}}

void uart_tx_byte(uint8_t b) //{{{
{
	int i,j;
	int write_len;
	int to_write_len;

	uart_tx_buffer[uart_tx_buffer_idx++] = b;

	// reached end of frame so write it
	if (uart_tx_buffer_idx > 3 &&
		uart_tx_buffer[uart_tx_buffer_idx-1] == UART_END_DELIM &&
		!(uart_tx_buffer[uart_tx_buffer_idx-2] == UART_ESC_DELIM &&
		uart_tx_buffer[uart_tx_buffer_idx-3] != UART_ESC_DELIM))
	{
		to_write_len = uart_tx_buffer_idx;
		while (to_write_len > 0)
		{
			if ((write_len = ftdi_write_data(ftdi, uart_tx_buffer + (uart_tx_buffer_idx - to_write_len), to_write_len)) < 0)
			{
				fprintf(stderr, "unable to write to ftdi device: %d (%s)\n", write_len, ftdi_get_error_string(ftdi));
				exit(EXIT_FAILURE);
			}

			// verbose
			verb_printf("uart_tx_byte: sent %s", "");
			for (i = 0; i < write_len; i++)
			{
				verb_printf(" %.2X", uart_tx_buffer[i + (uart_tx_buffer_idx - to_write_len)]);
			}
			verb_printf("%s\n", "");

			to_write_len -= write_len;
		}
		uart_tx_buffer_idx = 0;
	}
} //}}}

void uart_tx_bytes(uint8_t type, void* b, uint16_t len) //{{{
{
	uint16_t i;
	uint8_t* b_b = (uint8_t*)b;

	// verbose
	verb_printf("uart_tx_bytes: send%s", "");
	for (i = 0; i < len; i++)
	{
		verb_printf(" %.2X", b_b[i]);
	}
	verb_printf("%s\n", "");

	uart_tx_byte(UART_START_DELIM);
	uart_tx_byte(type);

	for (i = 0; i < len; i++)
	{
		if (b_b[i] == UART_START_DELIM || b_b[i] == UART_END_DELIM || b_b[i] == UART_ESC_DELIM)
		{
			uart_tx_byte(UART_ESC_DELIM);	
		}

		uart_tx_byte(b_b[i]);
	}

	uart_tx_byte(UART_END_DELIM);
} //}}}

void uart_init() //{{{
{
	int ret, i;
	uint8_t zeros[10];

	memset(zeros, 0, sizeof(zeros));

	if ((ftdi = ftdi_new()) == 0)
	{
		fprintf(stderr, "ftdi_new failed\n");
		exit(EXIT_FAILURE);
	}

	if ((ret = ftdi_usb_open(ftdi, 0x0403, 0x6001)) < 0)
	{
		fprintf(stderr, "unable to open ftdi device: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
		exit(EXIT_FAILURE);
	}

	if ((ret = ftdi_usb_reset(ftdi)) < 0)
	{
		fprintf(stderr, "unable to reset: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
		exit(EXIT_FAILURE);
	}

	if ((ret = ftdi_set_baudrate(ftdi, BAUD_RATE)) < 0)
	{
		fprintf(stderr, "unable to set baudrate: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
		exit(EXIT_FAILURE);
	}

	if ((ret = ftdi_set_line_property(ftdi, BITS_8, STOP_BIT_1, NONE)) < 0)
	{
		fprintf(stderr, "unable to set line properties: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
		exit(EXIT_FAILURE);
	}

	if ((ret = ftdi_setflowctrl(ftdi, SIO_DISABLE_FLOW_CTRL)) < 0)
	{
		fprintf(stderr, "unable to disable flow control: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
		exit(EXIT_FAILURE);
	}

	/* I am not sure why increasing the latency timer to 50ms makes it
	 * so that there are no more received frames with droped bytes in
	 * the middle, but it works so I am not going to complain.
         */
	ftdi_set_latency_timer(ftdi, 5);

	ftdi_usb_purge_buffers(ftdi);

	// on some libftdi installs the first write always fails, worst case this
	// is a bunch of start delimiters so it will have no effect
	ftdi_write_data(ftdi, zeros, sizeof(zeros));
} //}}}
