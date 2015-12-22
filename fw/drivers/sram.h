#ifndef SRAM_H
#define SRAM_H

#define USART_UCPHA_bm (1<<1)

#define USART_BSCALE_2000000BPS 0b0001 // 2's 0 
#define USART_BSEL_2000000BPS 0

#define SRAM_24BIT_ADDR
#define SRAM_SIZE_BYTES 65535

#define SRAM_CS_PIN_gm (1<<2)

#define SRAM_MODE_BYTE 0
#define SRAM_MODE_PAGE 1
#define SRAM_MODE_SEQ  2

#define SRAM_CMD_READ 0x03
#define SRAM_CMD_WRITE 0x02

#define USARTC1_XCK_PIN (1<<5)
#define USARTC1_TXD_PIN (1<<7)
#define USARTC1_RXD_PIN (1<<6)

void sram_init();

int sram_self_test();

void* sram_write(void* addr, const void* src, size_t len);
void* sram_read(void* dst, const void* addr, size_t len);

#endif
