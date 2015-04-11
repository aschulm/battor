#ifndef UART_H
#define UART_H

#include <stdio.h>

#define UART_START_DELIM 0x00
#define UART_END_DELIM 0x01
#define UART_ESC_DELIM 0x02

typedef enum UART_TYPE_enum
{
	UART_TYPE_CONTROL = 0x03,
	UART_TYPE_CONTROL_ACK,
	UART_TYPE_SAMPLES
} UART_TYPE_t;

#define UART_BUFFER_LEN 100

#define USARTD0_TXD_PIN (1<<3)
#define USARTD0_RXD_PIN (1<<2)

// computed with a graph based on the formulas in the datasheet and a 16MHz f_per 
#define USART_BSCALE_115200BPS 0b1100 // 2's -4
#define USART_BSEL_115200BPS 122 

#define USART_BSCALE_921600BPS 0b1001 // 2's -7
#define USART_BSEL_921600BPS 11

#define USART_BSCALE_400000BPS 0b0000 // 2's 0
#define USART_BSEL_400000BPS 4

#define USART_BSCALE_800000BPS 0b1111 // 2's -1
#define USART_BSEL_800000BPS 3

#define USART_BSCALE_1000000BPS 0b0000 // 2's 0 
#define USART_BSEL_1000000BPS 1

#define USART_BSCALE_2000000BPS 0b0001 // 2's 0 
#define USART_BSEL_2000000BPS 0

void uart_init();
void uart_tx_start(uint8_t type);
void uart_tx_end();
void uart_tx_bytes(void* b, uint16_t len);
uint8_t uart_rx_bytes(uint8_t* type, uint8_t* b, uint8_t b_len);
int uart_putchar(char c, FILE* stream);
int uart_getchar(FILE* stream);

#endif
