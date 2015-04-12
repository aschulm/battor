#ifndef SAMPLES_H
#define SAMPLES_H

#define SAMPLES_LEN 100

extern uint32_t g_samples_uart_seqnum;
extern uint16_t g_samples_read_file;
extern int16_t g_adca0[], g_adca1[], g_adcb0[], g_adcb1[];

void samples_uart_write(int16_t* v_s, int16_t* i_s, uint16_t len);
void samples_store_write(int16_t* v_s, int16_t* i_s);
void samples_store_read(uint16_t file);

#endif
