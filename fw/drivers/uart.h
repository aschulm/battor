#ifndef UART_H
#define UART_H

#include <stdio.h>

#define UART_BUFFER_LEN 20

#define USARTD0_TXD_PIN (1<<3)
#define USARTD0_RXD_PIN (1<<2)

// computed with a graph based on the formulas in the datasheet and a 16MHz f_per 
#define USART_BSCALE_115200BPS 0b1100 // 2's -4
#define USART_BSEL_115200BPS 122 

#define USART_BSCALE_921600BPS 0b1001 // 2's -7
#define USART_BSEL_921600BPS 11

#define USART_BSCALE_1000000BPS 0
#define USART_BSEL_1000000BPS 0

struct uart_hdr
{
	unsigned char delim[2];
	unsigned int seqnum;
	unsigned int len;
};

void uart_init();
void uart_tx_byte(unsigned char b);
void uart_tx_bytes(void* b, uint16_t len);
uint8_t uart_rx_bytes(uint8_t* b, uint8_t b_len);
int uart_putchar(char c, FILE* stream);
int uart_getchar(FILE* stream);

#endif
