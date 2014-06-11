#ifndef SAMPLES_H
#define SAMPLES_H

#define SAMPLES_LEN (800*2)

#define SAMPLES_DELIM0 0xBA
#define SAMPLES_DELIM1 0x77

extern uint8_t g_samples_uart_seqnum;
extern uint16_t g_samples_read_file;
extern uint8_t g_adca0[], g_adca1[], g_adcb0[], g_adcb1[];

void samples_uart_write(uint8_t* v_s, uint8_t* i_s, uint16_t len);
void samples_store_write(uint8_t* v_s, uint8_t* i_s);
void samples_store_read(uint16_t file);

#endif
