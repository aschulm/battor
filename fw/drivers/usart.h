#ifndef USART_H
#define USART_H

#define USART1_XCK_PIN (1<<5)
#define USART1_TXD_PIN (1<<7)
#define USART1_RXD_PIN (1<<6)

#define USART0_TXD_PIN (1<<3)
#define USART0_RXD_PIN (1<<2)

#define USART_UCPHA_bm (1<<1)

// computed with a graph based on the formulas in the datasheet and a 16MHz f_per 

#define USART_BSCALE_50000BPS 0b0011 // 2's 3
#define USART_BSEL_50000BPS 4

#define USART_BSCALE_100000BPS 0b0010 // 2's 2 
#define USART_BSEL_100000BPS 4

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

#define USART_BSCALE_2000000BPS 0b0000 // 2's 0 
#define USART_BSEL_2000000BPS 0

void usart_spi_txrx(USART_t* usart, const void* txd, void* rxd, uint16_t len);

#endif
