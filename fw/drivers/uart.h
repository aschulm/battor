#ifndef UART_H
#define UART_H

#include <stdio.h>

#define UART_TX_TIMEOUT_20US 40000

// these are defined by their pin on the FTDI chip
#define UART_RTS_bm (1<<5)
#define UART_RTS_bp 5
#define UART_CTS_bm (1<<4)

#define UART_START_DELIM 0x00
#define UART_END_DELIM 0x01
#define UART_ESC_DELIM 0x02

typedef enum UART_TYPE_enum
{
	UART_TYPE_CONTROL = 0x03,
	UART_TYPE_CONTROL_ACK,
	UART_TYPE_SAMPLES,
	UART_TYPE_PRINT,
} UART_TYPE_t;

#define UART_TX_BUFFER_LEN 2000
#define UART_RX_BUFFER_LEN 200 

void uart_init();
void uart_set_rts(uint8_t ready);
void uart_tx_start_prepare(uint8_t type);
void uart_tx_bytes_prepare(void* b, uint16_t len);
void uart_tx_end_prepare();
void uart_tx();
void uart_tx_dma();
void uart_tx_bytes(void* b, uint16_t len);
uint16_t uart_get_tx_buffer(uint8_t** tx_buffer);
uint16_t uart_set_tx_buffer(uint8_t* tx_buffer);
uint8_t uart_rx_is_pending();
uint8_t uart_rx_bytes(uint8_t* type, uint8_t* b, uint8_t b_len);
void uart_rx_flush();
uint32_t uart_rx_sample_count();
int uart_putchar(char c, FILE* stream);
int uart_getchar(FILE* stream);
uint8_t uart_tx_ready();
int uart_self_test();

#endif
