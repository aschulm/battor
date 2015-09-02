#ifndef SRAM_H
#define SRAM_H

#define SRAM_SIZE_BYTES 64000

#define SRAM_CS_PIN_gm (1<<2)

#define SRAM_MODE_BYTE 0
#define SRAM_MODE_PAGE 1
#define SRAM_MODE_SEQ  2

#define SRAM_CMD_READ 0x03
#define SRAM_CMD_WRITE 0x02

void sram_init();

int sram_self_test();

void* sram_write(void* addr, const void* src, size_t len);
void* sram_read(void* dst, const void* addr, size_t len);

#endif
