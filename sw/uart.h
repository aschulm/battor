#ifndef UART_H
#define UART_H

#include <stdlib.h>
#include <inttypes.h>

#define UART_START_DELIM 0x00
#define UART_END_DELIM 0x01
#define UART_ESC_DELIM 0x02

#define UART_RX_BUFFER_LEN 4096
#define UART_TX_BUFFER_LEN 4096

#define UART_RX_ATTEMPTS 10

typedef enum UART_TYPE_enum
{
	UART_TYPE_CONTROL = 0x03,
	UART_TYPE_CONTROL_ACK,
	UART_TYPE_SAMPLES
} UART_TYPE_t;

void uart_init();
int uart_rx_bytes(uint8_t* type, void* b, uint16_t b_len);
void uart_tx_bytes(uint8_t type, void* b, uint16_t b_len);

#endif
