#ifndef UART_H
#define UART_H

#include <stdlib.h>
#include <inttypes.h>

void uart_init();
void uart_read(void* b, uint16_t b_len);
void uart_write(void* b, uint16_t b_len);

#endif
