#ifndef SRAM_H
#define SRAM_H

#define SRAM_CS_PIN_gm (1<<2)

#define SRAM_MODE_BYTE 0
#define SRAM_MODE_PAGE 1
#define SRAM_MODE_SEQ  2

void sram_init();

int sram_self_test();

void sram_set_mode(int mode);
int sram_get_mode();

#endif
