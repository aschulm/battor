#ifndef UART_H
#define UART_H

#include <stdio.h>

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

#define UART_BUFFER_LEN 1000

void uart_init();
void uart_tx_start(uint8_t type);
void uart_tx_end();
void uart_tx_bytes(void* b, uint16_t len);
uint8_t uart_rx_bytes(uint8_t* type, uint8_t* b, uint8_t b_len);
void uart_rx_flush();
void uart_tx_bytes_dma(uint8_t type, void* b, uint16_t len);
int uart_putchar(char c, FILE* stream);
int uart_getchar(FILE* stream);
uint8_t uart_tx_ready();

#endif
