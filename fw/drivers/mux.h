#ifndef MUX_H
#define MUX_H

#define MUX_IN_bm (1<<0)

#define MUX_GND 0 
#define MUX_R   1

void mux_init();
void mux_select(uint8_t dir);

#endif
