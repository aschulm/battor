#ifndef SAMPLES_H
#define SAMPLES_H

#define SAMPLES_LEN 160
#define SAMPLES_CAL_BUFFERS 5
#define SAMPLES_UART_TX_TIMEOUT_20US 100000

typedef struct sample_
{
	int16_t v;
	int16_t i;
} sample;

extern sample g_adcb0[], g_adcb1[];

void samples_start();
void samples_stop();
void samples_end_calibration();
void samples_avg(sample* s);
void samples_ringbuf_write(sample* s);
int samples_uart_write(uint8_t just_prepare);
int samples_store_write();
void samples_store_read_uart_write(uint16_t file_num_to_open);

#endif
