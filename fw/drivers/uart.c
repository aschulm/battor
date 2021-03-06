#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>

#include "dma.h"
#include "usart.h"
#include "led.h"
#include "../interrupt.h"
#include "../error.h"
#include "../control.h"

#include "uart.h"

static volatile uint32_t frame_sample_count;
static volatile uint8_t uart_rx_buffer[UART_RX_BUFFER_LEN];
static volatile uint8_t uart_rx_buffer_write_idx;
static uint8_t uart_rx_buffer_read_idx;
static volatile uint8_t uart_tx_fc_ready;
static uint8_t* uart_tx_buffer;
static uint8_t uart_tx_buffer_default[UART_TX_BUFFER_LEN];
static uint16_t uart_tx_buffer_len;
static int uart_initialized = 0;

/* Receive UART byte interrupt
 *
 * do as little as possible in here because
 * we are operating close to the CPU clock rate
 */
ISR(USARTD0_RXC_vect) //{{{
{
	// UART error, just skip it
	if ((USARTD0.STATUS & USART_FERR_bm) > 0 || (USARTD0.STATUS & USART_BUFOVF_bm) > 0)
	{
		USARTD0.DATA;
		USARTD0.STATUS = 0;
	}
	else
	{
		// read bytes until USART fifo is empty
		while ((USARTD0.STATUS & USART_RXCIF_bm) > 0)
		{
			uint8_t tmp = USARTD0.DATA;

			// if byte is start byte then store sample count for it
			if (tmp == UART_START_DELIM && 
				(uart_rx_buffer_write_idx == 0 ||
				uart_rx_buffer[uart_rx_buffer_write_idx] != UART_ESC_DELIM))
				frame_sample_count = dma_get_sample_count();

			// store byte
			uart_rx_buffer[uart_rx_buffer_write_idx++] = tmp;
			if (uart_rx_buffer_write_idx >= UART_RX_BUFFER_LEN)
				uart_rx_buffer_write_idx = 0;
		}
	}
} //}}}

ISR(PORTD_INT0_vect) //{{{
{
	if ((PORTD.IN & UART_RTS_bm) > 0)
	{
		// pause
		dma_uart_tx_pause(1);
		uart_tx_fc_ready = 0;
	}
	else
	{
		// unpause
		dma_uart_tx_pause(0);
		uart_tx_fc_ready = 1;
	}
} //}}}

#ifdef DEBUG
	static FILE g_uart_stream = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);
#endif

void uart_init() //{{{
{
	interrupt_disable();

	// initalize hardware flow control gpios
	PORTD.OUT &= ~UART_RTS_bm;
	PORTD.DIR &= ~UART_RTS_bm;

	// start with CTS inidcating ready for data
	PORTD.OUT &= ~UART_CTS_bm;
	PORTD.DIR |= UART_CTS_bm;

	PORTD.INTCTRL = PORT_INT0LVL_HI_gc;
	// default pin behavior is to trigger on both edges
	PORTD.INT0MASK = UART_RTS_bm;

	PORTD.OUT |= USART0_TXD_PIN; // set the TXD pin high
	PORTD.DIR |= USART0_TXD_PIN; // set the TXD pin to output
	PORTD.DIR &= ~USART0_RXD_PIN; // set the RX pin to input

	// set baud rate with BSEL and BSCALE values
	usart_set_baud(&USARTD0, USART_BSEL_2000000BPS, USART_BSCALE_2000000BPS);

	USARTD0.CTRLC = USART_CMODE_ASYNCHRONOUS_gc | USART_PMODE_DISABLED_gc | USART_CHSIZE_8BIT_gc; // set transfer parameters
	USARTD0.CTRLA = USART_RXCINTLVL_HI_gc; // interrupt for receive 
	USARTD0.CTRLB |= USART_RXEN_bm; // enable receiver
	USARTD0.CTRLB |= USART_TXEN_bm; // enable transmitter

	// just to be safe, move the timer compare to another pin
	//PORTD.REMAP = (1<<3);

	uart_rx_buffer_write_idx = 0; // reset the receive buffer pointer
	uart_rx_buffer_read_idx = 0; // reset the receive buffer pointer

	uart_tx_fc_ready = 1; // start with not waiting for flow control

	uart_tx_buffer_len = 0; // start with a blank uart tx frame

	frame_sample_count = 0; // reset sample count for frame

	// set uart tx buffer to the local buffer
	uart_tx_buffer = uart_tx_buffer_default;

	// set stdout to uart so we can use printf for debugging
#ifdef DEBUG
	stdout = &g_uart_stream;
#endif
	
	interrupt_enable();

	uart_initialized = 1;
} //}}}

void uart_set_rts(uint8_t ready) //{{{
{
	// set the CTS pin on the FTDI to assert or deassert RTS
	if (ready)
		PORTD.OUT |= UART_CTS_bm;
	else
		PORTD.OUT &= ~UART_CTS_bm;
} //}}}

void uart_tx_start_prepare(uint8_t type) //{{{
{
	uart_tx_buffer[uart_tx_buffer_len++] = UART_START_DELIM;
	uart_tx_buffer[uart_tx_buffer_len++] = type;
} //}}}

void uart_tx_bytes_prepare(void* b, uint16_t len) ///{{{
{
	uint8_t* b_b = (uint8_t*)b;
	uint8_t* b_b_end = (uint8_t*)b + len;

	/* 
	 * This function has a loop with many compares so it 
	 * is optimized to execute in the fewest cycles possible
	 * by using pointer arithmatic rather than array indexing>
	 */

	uint8_t* tx_buffer_b = uart_tx_buffer + uart_tx_buffer_len;

	while (b_b < b_b_end)
	{
			if (*b_b <= UART_ESC_DELIM)
			*tx_buffer_b++ = UART_ESC_DELIM;

		*tx_buffer_b++ = *b_b++;
	}

	// update length of uart_tx_buffer based on pointers
	uart_tx_buffer_len = tx_buffer_b - uart_tx_buffer;
} //}}}

void uart_tx_end_prepare() //{{{
{
	uart_tx_buffer[uart_tx_buffer_len++] = UART_END_DELIM;
} //}}}

void uart_tx() //{{{
{
	uint16_t i;
	uint16_t tries = 0;

	for (i = 0; i < uart_tx_buffer_len; i++)
	{
		// wait for flow control
		while (!uart_tx_ready() &&
			tries++ < UART_TX_TIMEOUT_20US)
			_delay_us(20);
		
		if (tries >= UART_TX_TIMEOUT_20US)
			goto cleanup;

		loop_until_bit_is_set(USARTD0.STATUS, USART_DREIF_bp); // wait for tx buffer to empty
		USARTD0.DATA = uart_tx_buffer[i]; // put the data in the tx buffer
	}

cleanup:
	uart_tx_buffer_len = 0;
} //}}}

void uart_tx_dma() //{{{
{
	if (uart_tx_buffer_len > 0)
		dma_uart_tx(uart_tx_buffer, uart_tx_buffer_len);
	uart_tx_buffer_len = 0;
} //}}}

uint16_t uart_get_tx_buffer(uint8_t** tx_buffer) //{{{
{
	uint16_t tx_buffer_len = uart_tx_buffer_len;
	uart_tx_buffer_len = 0;

	*tx_buffer = uart_tx_buffer;

	return tx_buffer_len;
} //}}}

uint16_t uart_set_tx_buffer(uint8_t* tx_buffer) //{{{
{
	uint16_t tx_buffer_len = uart_tx_buffer_len;
	uart_tx_buffer_len = 0;

	uart_tx_buffer = tx_buffer;

	return tx_buffer_len;
} //}}}

inline uint8_t uart_rx_is_pending() //{{{
{
	return uart_rx_buffer_read_idx != uart_rx_buffer_write_idx;
} //}}}

static inline uint8_t rx_byte(uint8_t* b, uint8_t* read_idx) //{{{
{
	// no more bytes remaining
	if (*read_idx == uart_rx_buffer_write_idx)
		return 0;

	*b = uart_rx_buffer[(*read_idx)++];
	if (*read_idx >= UART_RX_BUFFER_LEN)
		*read_idx = 0;

	return 1;
} //}}}

uint8_t uart_rx_bytes(uint8_t* type, uint8_t* b, uint8_t b_len) //{{{
{
	uint8_t b_tmp = 0x00;

	uint8_t bytes_read;
	uint8_t start_found;
	uint8_t end_found;
	uint8_t esc_last;
	uint8_t start_last;

	// Create local copy of the UART RX buffer index
	uint8_t rx_buffer_read_idx = uart_rx_buffer_read_idx;
	// Initial state is retry byte, as this will be reversed in the pass of the parser
	uint8_t retry_byte = 1;

parser_begin:
	uart_rx_buffer_read_idx = rx_buffer_read_idx;
	bytes_read = 0;
	start_found = 0;
	end_found = 0;
	esc_last = 0;
	start_last = 0;
	retry_byte = !retry_byte;

	while (!(start_found && end_found))
	{
		if (!retry_byte)
		{
			if (!rx_byte(&b_tmp, &rx_buffer_read_idx))
			{
				// Error: No bytes remaining
				return 0;
			}
		}

		if (start_found && !end_found && esc_last)
		{
			// Got escaped byte
			if (!(b_tmp <= UART_ESC_DELIM))
			{
				// Error: Invalid escaped byte - toss bytes and reset parser
				goto parser_begin;
			}

			if (bytes_read < b_len)
			{
				b[bytes_read++] = b_tmp;
				esc_last = 0;
			}
			else
			{
				// Error: Frame too long - toss bytes and reset parser
				goto parser_begin;
			}
		}
		else if (start_found && !end_found && b_tmp == UART_ESC_DELIM)
		{
			// Got escape delimiter
			esc_last = 1;
		}
		else if (!start_found && !end_found && b_tmp == UART_START_DELIM)
		{
			// Got start delimiter
			start_found = 1;
			start_last = 1;
			bytes_read = 0;
		}
		else if (start_found && !end_found && b_tmp == UART_END_DELIM)
		{
			// Got end delimiter
			end_found = 1;
		}
		else if (start_found && !end_found && start_last && b_tmp > UART_ESC_DELIM)
		{
			// Got type after start delimiter
			*type = b_tmp;
			start_last = 0;
		}
		else if (start_found && !end_found && b_tmp > UART_ESC_DELIM && bytes_read < b_len)
		{
			// Got payload byte
			b[bytes_read++] = b_tmp;
		}
		// Error: Frame too long, or new byte results in invalid parser state - toss bytes and reset parser
		else
		{
			goto parser_begin;
		}

		// If a byte was being retried, after this pass of the parser the retry is now complete.
		retry_byte = 0;
	}

	// Successfully read frame.
	uart_rx_buffer_read_idx = rx_buffer_read_idx;
	return bytes_read;
} //}}}

uint32_t uart_rx_sample_count() //{{{
{
	return frame_sample_count;
} // }}}

void uart_rx_flush() //{{{
{
	uart_rx_buffer_read_idx = 0;
	uart_rx_buffer_write_idx = 0;
} //}}}

uint8_t uart_tx_ready() //{{{
{
	// check if RTS is blocking a write
	uart_tx_fc_ready = !((PORTD.IN & UART_RTS_bm) >> UART_RTS_bp);

	return uart_tx_fc_ready && dma_uart_tx_ready();
} //}}}

// for c FILE* interface
int uart_putchar(char c, FILE* stream) //{{{
{
	static int in_message = 0;

	if (!uart_initialized)
		return 0;

	if (!in_message)
	{
		uart_tx_start_prepare(UART_TYPE_PRINT);
		in_message = 1;
	}
	if (c == '\n' || c == '\0')
		in_message = 0;

	uart_tx_bytes_prepare(&c, 1);

	if (!in_message)
	{
		uart_tx_end_prepare();
		uart_tx();
	}

	return 0;
} //}}}

int uart_getchar(FILE* stream) //{{{
{
	return '\0'; // TODO implement this
} //}}}

int uart_self_test() //{{{
{
	printf("uart self test\n");

	uint8_t type;
	uint8_t buffer[200];
	uint8_t ret;

	printf("testing rx control message...");
	uint8_t test1[] = {
		CONTROL_TYPE_INIT,
		0x01,
		0x00,
		0x02,
		0x02
	};
	uint8_t test1_wire[] = {
		UART_START_DELIM,
		UART_TYPE_CONTROL,
		UART_ESC_DELIM, CONTROL_TYPE_INIT,
		UART_ESC_DELIM, 0x01,
		UART_ESC_DELIM, 0x00,
		UART_ESC_DELIM, 0x02,
		UART_ESC_DELIM, 0x02,
		UART_END_DELIM
	};
	memcpy((uint8_t*)uart_rx_buffer, test1_wire, sizeof(test1_wire));
	uart_rx_buffer_read_idx = 0;
	uart_rx_buffer_write_idx = sizeof(test1_wire);
	ret = uart_rx_bytes(&type, buffer, sizeof(buffer));
	if (ret != sizeof(test1) ||
		type != UART_TYPE_CONTROL ||
		memcmp(test1, buffer, sizeof(test1)) != 0 ||
		uart_rx_buffer_read_idx != sizeof(test1_wire))
	{
		printf("FAILED\n");
		return 1;
	}
	printf("PASSED\n");

	printf("testing end before start ignored...");
	uint8_t test2[] = {
		CONTROL_TYPE_INIT,
		0x01,
		0x01,
		0x00,
		0x01
	};
	uint8_t test2_wire[] = {
		0xFF,
		UART_END_DELIM,
		UART_START_DELIM,
		UART_TYPE_CONTROL,
		UART_ESC_DELIM, CONTROL_TYPE_INIT,
		UART_ESC_DELIM, 0x01,
		UART_ESC_DELIM, 0x01,
		UART_ESC_DELIM, 0x00,
		UART_ESC_DELIM, 0x01,
		UART_END_DELIM
	};
	memcpy((uint8_t*)uart_rx_buffer, test2_wire, sizeof(test2_wire));
	uart_rx_buffer_read_idx = 0;
	uart_rx_buffer_write_idx = sizeof(test2_wire);
	ret = uart_rx_bytes(&type, buffer, sizeof(buffer));
	if (ret != sizeof(test2) ||
		type != UART_TYPE_CONTROL ||
		memcmp(test2, buffer, sizeof(test2)) != 0 ||
		uart_rx_buffer_read_idx != sizeof(test2_wire))
	{
		printf("FAILED\n");
		return 2;
	}
	printf("PASSED\n");

	printf("testing start before start ignored...");
	uint8_t test3[] = {
		CONTROL_TYPE_INIT,
		0x01,
		0x02,
		0x02,
		0x01
	};
	uint8_t test3_wire[] = {
		UART_START_DELIM,
		UART_START_DELIM,
		UART_TYPE_CONTROL,
		UART_ESC_DELIM, CONTROL_TYPE_INIT,
		UART_ESC_DELIM, 0x01,
		UART_ESC_DELIM, 0x02,
		UART_ESC_DELIM, 0x02,
		UART_ESC_DELIM, 0x01,
		UART_END_DELIM
	};
	memcpy((uint8_t*)uart_rx_buffer, test3_wire, sizeof(test3_wire));
	uart_rx_buffer_read_idx = 0;
	uart_rx_buffer_write_idx = sizeof(test3_wire);
	ret = uart_rx_bytes(&type, buffer, sizeof(buffer));
	if (ret != sizeof(test3) ||
		type != UART_TYPE_CONTROL ||
		memcmp(test3, buffer, sizeof(test3)) != 0 ||
		uart_rx_buffer_read_idx != sizeof(test3_wire))
	{
		printf("FAILED\n");
		return 3;
	}
	printf("PASSED\n");

	printf("testing frame, partial frame, frame...");
	uint8_t test4a[] = {
		CONTROL_TYPE_INIT,
		0x00,
		0x00,
		0x00,
		0x00
	};
	uint8_t test4b[] = {
		CONTROL_TYPE_INIT,
		0x01,
		0x01,
		0x02,
		0x02
	};
	uint8_t test4_wire[] = {
		UART_START_DELIM,
		UART_TYPE_CONTROL,
		UART_ESC_DELIM, CONTROL_TYPE_INIT,
		UART_ESC_DELIM, 0x00,
		UART_ESC_DELIM, 0x00,
		UART_ESC_DELIM, 0x00,
		UART_ESC_DELIM, 0x00,
		UART_END_DELIM,
		UART_START_DELIM,
		UART_TYPE_CONTROL,
		UART_ESC_DELIM, CONTROL_TYPE_INIT,
		UART_ESC_DELIM, 0x00,
		UART_START_DELIM,
		UART_TYPE_CONTROL,
		UART_ESC_DELIM, CONTROL_TYPE_INIT,
		UART_ESC_DELIM, 0x01,
		UART_ESC_DELIM, 0x01,
		UART_ESC_DELIM, 0x02,
		UART_ESC_DELIM, 0x02,
		UART_END_DELIM,
	};
	memcpy((uint8_t*)uart_rx_buffer, test4_wire, sizeof(test4_wire));
	uart_rx_buffer_read_idx = 0;
	uart_rx_buffer_write_idx = sizeof(test4_wire);
	ret = uart_rx_bytes(&type, buffer, sizeof(buffer));
	if (ret != sizeof(test4a) ||
		type != UART_TYPE_CONTROL ||
		memcmp(test4a, buffer, sizeof(test4a)) != 0)
	{
		printf("FAILED - first frame incorrect\n");
		return 4;
	}
	ret = uart_rx_bytes(&type, buffer, sizeof(buffer));
	if (ret != sizeof(test4b) ||
		type != UART_TYPE_CONTROL ||
		memcmp(test4b, buffer, sizeof(test4b)) != 0)
	{
		printf("FAILED - second frame incorrect\n");
		return 4;
	}
	printf("PASSED\n");

	printf("testing partial frame skipped then read...");
	uint8_t test5[] = {
		CONTROL_TYPE_INIT,
		0x02,
		0x02,
		0x01,
		0x01
	};
	uint8_t test5a_wire[] = {
		UART_START_DELIM,
		UART_TYPE_CONTROL,
		UART_ESC_DELIM, CONTROL_TYPE_INIT,
		UART_ESC_DELIM, 0x02,
	};
	uint8_t test5b_wire[] = {
		UART_ESC_DELIM, 0x02,
		UART_ESC_DELIM, 0x01,
		UART_ESC_DELIM, 0x01,
		UART_END_DELIM,
	};
	memcpy((uint8_t*)uart_rx_buffer, test5a_wire, sizeof(test5a_wire));
	uart_rx_buffer_read_idx = 0;
	uart_rx_buffer_write_idx = sizeof(test5a_wire);
	ret = uart_rx_bytes(&type, buffer, sizeof(buffer));
	if (ret != 0)
	{
		printf("FAILED - first read did not fail\n");
		return 5;
	}
	memcpy((uint8_t*)uart_rx_buffer + uart_rx_buffer_write_idx, test5b_wire, sizeof(test5b_wire));
	uart_rx_buffer_write_idx += sizeof(test5b_wire);
	ret = uart_rx_bytes(&type, buffer, sizeof(buffer));
	if (ret != sizeof(test5) ||
		type != UART_TYPE_CONTROL ||
		memcmp(test5, buffer, sizeof(test5)) != 0)
	{
		printf("FAILED - second read did not succeed\n");
		return 5;
	}
	printf("PASSED\n");

	return 0;
} //}}}
