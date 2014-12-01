#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <libftdi1/ftdi.h>

#include "uart.h"
#include "params.h"
#include "samples.h"

struct ftdi_context *ftdi;

void uart_init() //{{{
{
	speed_t baud_rate = BAUD_RATE;
	int ret;
	struct ftdi_version_info version;
	if ((ftdi = ftdi_new()) == 0)
	{
		fprintf(stderr, "ftdi_new failed\n");
		exit(EXIT_FAILURE);
	}
	if ((ret = ftdi_usb_open(ftdi, 0x0403, 0x6011)) < 0)
	{
		fprintf(stderr, "unable to open ftdi device: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
		exit(EXIT_FAILURE);
	}

	ftdi_set_baudrate(ftdi, BAUD_RATE);
	ftdi_set_line_property(ftdi, BITS_8, STOP_BIT_1, NONE);
} //}}}

void uart_read(void* b, uint16_t b_len) //{{{
{
	uint8_t* b_b = (uint8_t*)b;
	int read_len;
	int to_read_len = b_len;
	while (to_read_len > 0)
	{
		if ((read_len = ftdi_read_data(ftdi, b_b + (b_len - to_read_len), to_read_len)) < 0)
		{
			perror("ftdi_read_data()");
			exit(EXIT_FAILURE);
		}
		to_read_len -= read_len;
	}
} //}}}

void uart_write(void* b, uint16_t b_len) //{{{
{
	uint8_t* b_b = (uint8_t*)b;
	int write_len;
	int to_write_len = b_len;
	while (to_write_len > 0)
	{
		if ((write_len = ftdi_write_data(ftdi, b_b + (b_len - to_write_len), to_write_len)) < 0)
			perror("write()");
		to_write_len -= write_len;
	}
} //}}}
