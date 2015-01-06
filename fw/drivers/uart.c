#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/interrupt.h>

#include "uart.h"
#include "led.h"
#include "../interrupt.h"
#include "../error.h"

// receive byte interrupt, do as little as possible in here because we are operating close to the CPU clock rate
static volatile uint8_t uart_rx_buffer[UART_BUFFER_LEN];
static volatile uint8_t uart_rx_buffer_write_idx;
static volatile uint8_t uart_rx_buffer_read_idx;
ISR(USARTD0_RXC_vect) //{{{
{
	if ((USARTD0.STATUS & USART_FERR_bm) > 0 || (USARTD0.STATUS & USART_BUFOVF_bm) > 0)
		halt(0);

	// read bytes until USART fifo is empty
	while ((USARTD0.STATUS & USART_RXCIF_bm) > 0)
	{
		uart_rx_buffer[uart_rx_buffer_write_idx++] = USARTD0.DATA;
		if (uart_rx_buffer_write_idx >= UART_BUFFER_LEN)
			uart_rx_buffer_write_idx = 0;
	}
	interrupt_set(INTERRUPT_UART_RX);
} //}}}

#ifdef DEBUG
	static FILE g_uart_stream = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);
#endif

void uart_init() //{{{
{
	interrupt_disable();

	PORTD.OUT |= USARTD0_TXD_PIN; // set the TXD pin high
	PORTD.DIR |= USARTD0_TXD_PIN; // set the TXD pin to output
	PORTD.DIR &= ~USARTD0_RXD_PIN; // set the RX pin to input

	// set baud rate with BSEL and BSCALE values
#ifdef DEBUG
	USARTD0.BAUDCTRLB = USART_BSCALE_115200BPS << USART_BSCALE_gp;
	USARTD0.BAUDCTRLA = USART_BSEL_115200BPS & USART_BSEL_gm;
#else
	USARTD0.BAUDCTRLB = USART_BSCALE_800000BPS << USART_BSCALE_gp;
	USARTD0.BAUDCTRLA = USART_BSEL_800000BPS & USART_BSEL_gm;
#endif

	USARTD0.CTRLC = USART_CMODE_ASYNCHRONOUS_gc | USART_PMODE_DISABLED_gc | USART_CHSIZE_8BIT_gc; // set transfer parameters
	USARTD0.CTRLA = USART_RXCINTLVL_HI_gc; // interrupt for receive 
	USARTD0.CTRLB |= USART_RXEN_bm; // enable receiver
	USARTD0.CTRLB |= USART_TXEN_bm; // enable transmitter

	// just to be safe, move the timer compare to another pin
	//PORTD.REMAP = (1<<3);

	uart_rx_buffer_write_idx = 0; // reset the receive buffer pointer
	uart_rx_buffer_read_idx = 0; // reset the receive buffer pointer

	// set stdout to uart so we can use printf for debugging
#ifdef DEBUG
	stdout = &g_uart_stream;
#endif
	
	interrupt_enable();
} //}}}

inline void uart_tx_byte(uint8_t b) //{{{
{
	interrupt_disable();
	loop_until_bit_is_set(USARTD0.STATUS, USART_DREIF_bp); // wait for tx buffer to empty
	USARTD0.DATA = b; // put the data in the tx buffer
	interrupt_enable();
} //}}}

void uart_tx_start(uint8_t type) //{{{
{
	uart_tx_byte(UART_START_DELIM);
	uart_tx_byte(type);
} //}}}

void uart_tx_end() //{{{
{
	uart_tx_byte(UART_END_DELIM);
} //}}}

void uart_tx_bytes(void* b, uint16_t len) //{{{
{
	uint16_t i;
	uint8_t* b_b = (uint8_t*)b;

	for (i = 0; i < len; i++)
	{
		if (b_b[i] == UART_START_DELIM || b_b[i] == UART_END_DELIM || b_b[i] == UART_ESC_DELIM)
		{
			uart_tx_byte(UART_ESC_DELIM);	
		}

		uart_tx_byte(b_b[i]);
	}
} //}}}

inline uint8_t uart_rx_byte() //{{{
{
	uint8_t ret;

	// wait for byte to arrive
	while (uart_rx_buffer_read_idx == uart_rx_buffer_write_idx);

	ret = uart_rx_buffer[uart_rx_buffer_read_idx++];
	if (uart_rx_buffer_read_idx >= UART_BUFFER_LEN)
		uart_rx_buffer_read_idx = 0;

	return ret;
} //}}}

uint8_t uart_rx_bytes(uint8_t* type, uint8_t* b, uint8_t b_len) //{{{
{
	uint8_t b_tmp;
	uint8_t bytes_read = 0;
	uint8_t start_found = 0;
	uint8_t end_found = 0;

	while (!(start_found && end_found))
	{
		b_tmp = uart_rx_byte();

		if (b_tmp == UART_ESC_DELIM)
		{
			b_tmp = uart_rx_byte();

			if (bytes_read < b_len)
			{
				b[bytes_read++] = b_tmp;
			}
		}
		else if (b_tmp == UART_START_DELIM)
		{
			start_found = 1;
			bytes_read = 0;

			*type = uart_rx_byte();
		}
		else if (b_tmp == UART_END_DELIM)
		{
			end_found = 1;
		}
		else if (bytes_read < b_len)
		{
			b[bytes_read++] = b_tmp;
		}
	}

	return bytes_read;
} //}}}

// for c FILE* interface
int uart_putchar(char c, FILE* stream) //{{{
{
	if (c == '\n')
		uart_putchar('\r', stream);
	uart_tx_byte(c);
	return 0;
} //}}}

int uart_getchar(FILE* stream) //{{{
{
	return '\0'; // TODO implement this
} //}}}
