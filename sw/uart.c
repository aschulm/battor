#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>

#include "uart.h"
#include "params.h"
#include "samples.h"

static int fd;

void uart_init() //{{{
{
	struct termios tio;
	speed_t baud_rate = BAUD_RATE;
	
#ifdef __APPLE__
	if ((fd = open(BATTOR_UART_PATH_APPLE, O_RDWR | O_NOCTTY)) < 0)
#elif __linux__
	if ((fd = open(BATTOR_UART_PATH_LINUX, O_RDWR | O_NOCTTY)) < 0)
#endif
	{
		perror("open()");
		exit(EXIT_FAILURE);
	}
	if (tcgetattr(fd, &tio) == -1)
	{
		perror("tcgetattr()");
		exit(EXIT_FAILURE);
	}

#ifdef __APPLE__
	cfsetspeed(&tio, B115200); // don't care, we  the baud rate anyway
	cfmakeraw(&tio); // read one byte or timeout
	tio.c_cc[VMIN] = 1; // 
	tio.c_cc[VTIME] = 10; //
	tio.c_cflag = CS8; // 8-bit mode
#elif __linux__
	cfsetspeed(&tio, B38400); // don't care, we  the baud rate anyway
	cfmakeraw(&tio); // read one byte or timeout
	tio.c_cflag |= B38400 | CS8;
#endif

	tcsetattr(fd, TCSANOW, &tio); // set the options now

#if __APPLE__
	#include <IOKit/serial/ioss.h>
	#include <sys/ioctl.h>
	if (ioctl(fd, IOSSIOSPEED, &baud_rate) == -1) // set a non-standard baud rate on os X
	{
		perror("ioctl()");
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
} //}}}

void uart_read(void* b, uint16_t b_len) //{{{
{
	uint8_t* b_b = (uint8_t*)b;
	int read_len;
	int to_read_len = b_len;
	while (to_read_len > 0)
	{
		if ((read_len = read(fd, b_b + (b_len - to_read_len), to_read_len)) < 0)
			perror("read()");
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
		if ((write_len = write(fd, b_b + (b_len - to_write_len), to_write_len)) < 0)
			perror("write()");
		to_write_len -= write_len;
	}
} //}}}
