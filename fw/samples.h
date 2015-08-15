#ifndef SAMPLES_H
#define SAMPLES_H

#define SAMPLES_LEN 500

typedef struct sample_
{
	int16_t v;
	int16_t i;
} sample;

extern uint32_t g_samples_uart_seqnum;
extern sample g_adcb0[], g_adcb1[];

void samples_uart_write(sample* s, uint16_t len);
void samples_store_write(sample* s);
uint16_t samples_store_read_next(sample* s);

#endif
