#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/interrupt.h>

#include "uart.h"
#include "../interrupt.h"
#include "../error.h"

// receive byte interrupt, do as little as possible in here because we are operating close to the CPU clock rate
static volatile uint8_t uart_rx_buffer[UART_BUFFER_LEN];
static volatile uint8_t uart_rx_buffer_write_idx;
static volatile uint8_t uart_rx_buffer_read_idx;
ISR(USARTD0_RXC_vect) //{{{
{
	uart_rx_buffer[uart_rx_buffer_write_idx++] = USARTD0.DATA;
	if (uart_rx_buffer_write_idx > UART_BUFFER_LEN)
		halt();
	interrupt_set(INTERRUPT_UART_RX);
} //}}}

static FILE g_uart_stream = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);

void uart_init() //{{{
{
	PORTD.DIR |= USARTD0_TXD_PIN; // set the TXD pin to output
	PORTD.OUT |= USARTD0_TXD_PIN; // set the TXD pin high
	PORTD.DIR &= ~USARTD0_RXD_PIN; // set the RX pin to input

	// set baud rate with BSEL and BSCALE values
//#ifdef DEBUG
	USARTD0.BAUDCTRLB = USART_BSCALE_115200BPS << USART_BSCALE_gp;
	USARTD0.BAUDCTRLA = USART_BSEL_115200BPS & USART_BSEL_gm;
//#else
	USARTD0.BAUDCTRLB = USART_BSCALE_921600BPS << USART_BSCALE_gp;
	USARTD0.BAUDCTRLA = USART_BSEL_921600BPS & USART_BSEL_gm;
//#endif

	USARTD0.CTRLC = USART_CMODE_ASYNCHRONOUS_gc | USART_PMODE_DISABLED_gc | USART_CHSIZE_8BIT_gc; // set transfer parameters
	USARTD0.CTRLA = USART_RXCINTLVL_MED_gc; // interrupt for receive 
	USARTD0.CTRLB = USART_RXEN_bm | USART_TXEN_bm; // enable receiver and transmitter

	uart_rx_buffer_write_idx = 0; // reset the receive buffer pointer
	uart_rx_buffer_read_idx = 0; // reset the receive buffer pointer

	// set stdout to uart so we can use printf for debugging
#ifdef DEBUG
	stdout = &g_uart_stream;
#endif
} //}}}

void uart_tx_byte(unsigned char b) //{{{
{
	loop_until_bit_is_set(USARTD0.STATUS, USART_DREIF_bp); // wait for tx buffer to empty
	USARTD0.DATA = b; // put the data in the tx buffer
} //}}}

void uart_tx_bytes(void* b, uint16_t len) //{{{
{
	uint16_t i;
	uint8_t* b_b = (uint8_t*)b;
	for (i = 0; i < len; i++)
	{
		loop_until_bit_is_set(USARTD0.STATUS, USART_DREIF_bp); // wait for tx buffer to empty
		USARTD0.DATA = b_b[i]; // put the data in the tx buffer
	}
} //}}}

uint8_t uart_rx_bytes(uint8_t* b, uint8_t b_len) //{{{
{
	uint8_t bytes_read = 0;
	interrupt_disable();
	while (bytes_read < b_len && uart_rx_buffer_read_idx < uart_rx_buffer_write_idx)
		b[bytes_read++] = uart_rx_buffer[uart_rx_buffer_read_idx++];

	// read up to what's written, start from the beginning
	if (uart_rx_buffer_read_idx == uart_rx_buffer_write_idx)
	{
		uart_rx_buffer_read_idx = 0;
		uart_rx_buffer_write_idx = 0;
	}
	interrupt_enable();
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
