#include "common.h"
#include "uart.h"
#include "params.h"
#include "samples.h"

static int fd;
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

		// try non-blocking read
		if ((uart_rx_buffer_write_idx = read(fd, uart_rx_buffer, sizeof(uart_rx_buffer))) <= 0)
		{
			vverb_printf("uart_rx_byte: read() ret:%d\n", uart_rx_buffer_write_idx);

			// read failed or did not return any data, wait a bit and try again
			struct timespec to_sleep = {0, UART_READ_SLEEP_NS};
			while (nanosleep(&to_sleep, &to_sleep) < 0);
			continue;
		}
		vverb_printf("uart_rx_byte: read() ret:%d\n", uart_rx_buffer_write_idx);
		uart_rx_buffer_read_idx = 0;

		vverb_printf("uart_rx_byte: len:%d", uart_rx_buffer_write_idx);
		for (i = 0; i < uart_rx_buffer_write_idx; i++)
		{
			vverb_printf(" %.2X", uart_rx_buffer[i]);
		}
		vverb_printf("%s\n", "");
	}

	*b = uart_rx_buffer[uart_rx_buffer_read_idx++];
	vverb_printf("uart_rx_byte: read:0x%.2X read_idx:%d\n", *b, uart_rx_buffer_read_idx);
	return uart_rx_buffer_write_idx;
} //}}}

int uart_rx_bytes(uint8_t* type, void* b, uint16_t b_len) //{{{
{
	int i, ret;
	uint8_t* b_b = NULL;
	uint16_t b_b_len = 0;
	uint8_t b_print[UART_RX_BUFFER_LEN];
	uint8_t b_tmp;

	uint8_t start_found = 0;
	uint8_t end_found = 0;
	uint8_t esc_last = 0;
	uint8_t start_last = 0;
	uint16_t bytes_read = 0;

uart_rx_bytes_start:
	start_found = end_found = bytes_read = 0;
	b_b = (uint8_t*)b;
	b_b_len = b_len;

	vverb_printf("uart_rx_bytes: begin\n%s", "");

	while (!(start_found && end_found))
	{
		if (uart_rx_byte(&b_tmp) <= 0)
		{
			vverb_printf("uart_rx_bytes: failed to get byte\n%s", "");
			return -1;
		}
		
		if (esc_last)
		{
			if (bytes_read < b_b_len)
			{
				vverb_printf("uart_rx_bytes: got escape --- stored escaped byte 0x%X\n", b_tmp);
				b_b[bytes_read++] = b_tmp;
			}

			esc_last = 0;
		}
		else if (b_tmp == UART_ESC_DELIM)
		{
			vverb_printf("uart_rx_bytes: got escape\n%s", "");
			esc_last = 1;
		}
		else if (b_tmp == UART_START_DELIM)
		{
			start_found = 1;
			start_last = 1;
			bytes_read = 0;
		}
		else if (b_tmp == UART_END_DELIM)
		{
			vverb_printf("uart_rx_byte: got END\n%s", "");
			end_found = 1;
		}
		else if (start_last)
		{
			*type = b_tmp;
			vverb_printf("uart_rx_bytes: got START type 0x%X\n", *type);

			// frame is a print type, so swap in print buffer
			if (*type == UART_TYPE_PRINT) 
			{
				b_b = b_print;
				b_b_len = sizeof(b_print);
			}
				
			start_last = 0;
		}
		else if (bytes_read < b_b_len)
		{
			if (!start_found) {
				vverb_printf("uart_rx_byte: got data before START 0x%X\n", b_tmp);
			} else {
				vverb_printf("uart_rx_byte: got byte 0x%X\n", b_tmp);
			 	b_b[bytes_read++] = b_tmp;
			}
		}
	}

	// verbose
	if (*type == UART_TYPE_PRINT) 
	{
		b_b[bytes_read] = '\0';
		fprintf(stderr, "[DEBUG] %s", b_b);
		goto uart_rx_bytes_start;
	} 
	else 
	{
		verb_printf("uart_rx_bytes: recv%s", "");
		for (i = 0; i < bytes_read; i++)
		{
			verb_printf(" %.2X", b_b[i]);
		}
		verb_printf("%s\n", "");
	}

	return bytes_read;
} //}}}

int uart_tx_byte(uint8_t b) //{{{
{
	int i,j, attempt = 0;
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
			if (attempt++ >= UART_TX_ATTEMPTS)
			{
				vverb_printf("uart_tx_byte: timeout\n%s", "");
				return -1;
			}

			if ((write_len = write(fd, uart_tx_buffer + (uart_tx_buffer_idx - to_write_len), to_write_len)) < 0)
			{
				vverb_printf("uart_tx_byte: write() ret:%d\n", write_len);

				// write failed, wait a bit and try again
				struct timespec to_sleep = {0, UART_WRITE_SLEEP_NS};
				while (nanosleep(&to_sleep, &to_sleep) < 0);
			}
			else
			{
				// verbose
				verb_printf("uart_tx_byte: sent %s", "");
				for (i = 0; i < write_len; i++)
				{
					verb_printf(" %.2X", uart_tx_buffer[i + (uart_tx_buffer_idx - to_write_len)]);
				}
				verb_printf("%s\n", "");

				to_write_len -= write_len;
			}
		}
		uart_tx_buffer_idx = 0;
	}
	return 0;
} //}}}

int uart_tx_bytes(uint8_t type, void* b, uint16_t len) //{{{
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

	if (uart_tx_byte(UART_START_DELIM) < 0)
		return -1;
	if (uart_tx_byte(type) < 0)
		return -1;

	for (i = 0; i < len; i++)
	{
		if (b_b[i] == UART_START_DELIM ||
			b_b[i] == UART_END_DELIM ||
			b_b[i] == UART_ESC_DELIM)
		{
			if (uart_tx_byte(UART_ESC_DELIM) < 0)
				return -1;
		}

		if (uart_tx_byte(b_b[i]) < 0)
			return -1;
	}

	if (uart_tx_byte(UART_END_DELIM) < 0)
		return -1;
	return 0;
} //}}}

void uart_init(char* tty) //{{{
{
	struct termios tio;
	speed_t baud_rate = BAUD_RATE;
	
	if ((fd = open(tty, O_RDWR | O_NOCTTY | O_NONBLOCK)) < 0)
	{
		perror("open()");
		exit(EXIT_FAILURE);
	}

	if (tcgetattr(fd, &tio) == -1)
	{
		perror("tcgetattr()");
		exit(EXIT_FAILURE);
	}

	// set parameters
	cfmakeraw(&tio); // read one byte or timeout
	tio.c_cc[VMIN] = 1; // 
	tio.c_cc[VTIME] = 0; //
  tio.c_iflag &= ~(IXON | IXOFF | IXANY); // disable software flow control
  tio.c_cflag |= CRTSCTS; // enable hardware flow control
  tio.c_cflag &= ~(CSTOPB | PARENB | CSIZE);
	tio.c_cflag |= CS8 | CREAD; // 8-bit mode
#if __APPLE__
	cfsetspeed(&tio, B115200); // don't care, we  the baud rate anyway
#elif __linux__
	cfsetspeed(&tio, B38400); // linux needs this for custom baud rate
#elif __CYGWIN__
	cfsetspeed(&tio, BAUD_RATE_CYGWIN); // CYGWIN can handle the full baud rate
#endif
	tcsetattr(fd, TCSANOW, &tio); // set the options now

	// set baud rate
#if __APPLE__
	#include <IOKit/serial/ioss.h>
	#include <sys/ioctl.h>
	if (ioctl(fd, IOSSIOSPEED, &baud_rate) < 0) // set a non-standard baud rate on os X
	{
		perror("ioctl(IOSSIOSPEED)");
		exit(EXIT_FAILURE);
	}
#elif __linux__
	#include <linux/serial.h>
	#include <asm-generic/ioctls.h>
	struct serial_struct ss;
	ioctl(fd, TIOCGSERIAL, &ss);
	ss.flags |= ASYNC_SPD_CUST;
	ss.custom_divisor = ss.baud_base / BAUD_RATE;
	ioctl(fd, TIOCSSERIAL, &ss);
#endif

	// set latency (this doesn't seem to work)
#if __APPLE__
	unsigned long ms = 1;
	if (ioctl(fd, IOSSDATALAT, &ms) < 0)
	{
		perror("ioctl(IOSSDATALAT)");
		exit(EXIT_FAILURE);
	}
#endif

	tcflush(fd, TCIOFLUSH); // flush the buffers in the FTDI
} //}}}
