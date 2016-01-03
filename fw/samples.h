#ifndef SAMPLES_H
#define SAMPLES_H

#define SAMPLES_LEN 50 

typedef struct sample_
{
	int16_t v;
	int16_t i;
} sample;

extern sample g_adcb0[], g_adcb1[];
extern uint8_t g_samples_calibrated;

void samples_init();
void samples_start();
void samples_stop();
void samples_end_calibration();
void samples_ringbuf_write(sample* s, uint16_t len);
void samples_uart_write();
void samples_store_write();
uint16_t samples_store_read_next(sample* s);

#endif
