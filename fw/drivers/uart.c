#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/interrupt.h>

#include "dma.h"
#include "usart.h"
#include "led.h"
#include "../interrupt.h"
#include "../error.h"

#include "uart.h"

// receive byte interrupt, do as little as possible in here because we are operating close to the CPU clock rate
static volatile uint8_t uart_rx_buffer[UART_BUFFER_LEN];
static volatile uint8_t uart_rx_buffer_write_idx;
static volatile uint8_t uart_rx_buffer_read_idx;
static volatile uint8_t uart_tx_fc_ready = 0;
static uint8_t uart_tx_buffer[UART_BUFFER_LEN];
static int uart_initialized = 0;

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
			// store byte
			uart_rx_buffer[uart_rx_buffer_write_idx++] = tmp;
			if (uart_rx_buffer_write_idx >= UART_BUFFER_LEN)
				uart_rx_buffer_write_idx = 0;

			// only interrupt if a byte actually came in
			interrupt_set(INTERRUPT_UART_RX);
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
	USARTD0.BAUDCTRLB = USART_BSCALE_2000000BPS << USART_BSCALE_gp;
	USARTD0.BAUDCTRLA = USART_BSEL_2000000BPS & USART_BSEL_gm;

	USARTD0.CTRLC = USART_CMODE_ASYNCHRONOUS_gc | USART_PMODE_DISABLED_gc | USART_CHSIZE_8BIT_gc; // set transfer parameters
	USARTD0.CTRLA = USART_RXCINTLVL_HI_gc; // interrupt for receive 
	USARTD0.CTRLB |= USART_RXEN_bm; // enable receiver
	USARTD0.CTRLB |= USART_TXEN_bm; // enable transmitter

	// just to be safe, move the timer compare to another pin
	//PORTD.REMAP = (1<<3);

	uart_rx_buffer_write_idx = 0; // reset the receive buffer pointer
	uart_rx_buffer_read_idx = 0; // reset the receive buffer pointer

	uart_tx_fc_ready = 1; // start with not waiting for flow control

	// set stdout to uart so we can use printf for debugging
#ifdef DEBUG
	stdout = &g_uart_stream;
#endif
	
	interrupt_enable();

	uart_initialized = 1;
} //}}}

inline void uart_tx_byte(uint8_t b) //{{{
{
	// wait for flow control
	while (!uart_tx_fc_ready);

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
		if (b_b[i] <= UART_ESC_DELIM)
			uart_tx_byte(UART_ESC_DELIM);	

		uart_tx_byte(b_b[i]);
	}
} //}}}

void uart_tx_bytes_dma(uint8_t type, void* b, uint16_t len) //{{{
{
	uint16_t b_i = 0, tx_buffer_i = 0;
	uint8_t* b_b = (uint8_t*)b;

	// prepare frame
	uart_tx_buffer[tx_buffer_i++] = UART_START_DELIM;
	uart_tx_buffer[tx_buffer_i++] = type;
	for (b_i = 0; b_i < len; b_i++)
	{
		if (b_b[b_i] <= UART_ESC_DELIM)
			uart_tx_buffer[tx_buffer_i++] = UART_ESC_DELIM;

		uart_tx_buffer[tx_buffer_i++] = b_b[b_i];
	}
	uart_tx_buffer[tx_buffer_i++] = UART_END_DELIM;

	// transmit with DMA
	dma_uart_tx(uart_tx_buffer, tx_buffer_i);
} //}}}

inline uint8_t uart_rx_byte() //{{{
{
	uint8_t ret;

	// wait for byte to arrive
	while (uart_rx_buffer_read_idx == uart_rx_buffer_write_idx);

	interrupt_disable();
	ret = uart_rx_buffer[uart_rx_buffer_read_idx++];
	if (uart_rx_buffer_read_idx >= UART_BUFFER_LEN)
		uart_rx_buffer_read_idx = 0;
	interrupt_enable();

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

    if (!in_message) {
        uart_tx_start(UART_TYPE_PRINT);
        in_message = 1;
    }
	if (c == '\n' || c == '\0') {
        in_message = 0;
    }

	uart_tx_byte(c);

    if (!in_message) {
        uart_tx_end();
    }

	return 0;
} //}}}

int uart_getchar(FILE* stream) //{{{
{
	return '\0'; // TODO implement this
} //}}}
