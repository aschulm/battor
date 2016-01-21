#ifndef SRAM_H
#define SRAM_H

#define SRAM_24BIT_ADDR
#define SRAM_SIZE_BYTES 125000

#define SRAM_CS_PIN_gm (1<<2)

#define SRAM_MODE_BYTE 0
#define SRAM_MODE_PAGE 1
#define SRAM_MODE_SEQ  2

#define SRAM_CMD_READ 0x03
#define SRAM_CMD_WRITE 0x02

void sram_init();

int sram_self_test();

uint32_t sram_write(uint32_t addr, const uint32_t src, uint32_t len);
uint32_t sram_read(uint32_t dst, const uint32_t addr, uint32_t len);

#endif
