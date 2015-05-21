#ifndef SAMPLES_H
#define SAMPLES_H

#define SAMPLES_LEN 1000

extern uint32_t g_samples_uart_seqnum;
extern int16_t g_adca0[], g_adca1[], g_adcb0[], g_adcb1[];

void samples_uart_write(int16_t* v_s, int16_t* i_s, uint16_t len);
void samples_store_write(int16_t* v_s, int16_t* i_s);
uint16_t samples_store_read_next(int16_t* v_s, int16_t* i_s);

#endif
