#ifndef SAMPLES_H
#define SAMPLES_H

#define SAMPLES_LEN 128
#define SAMPLES_UART_TX_TIMEOUT_20US 100000
#define SAMPLES_LEN_PER_CAL_FRAME 5
#define SAMPLES_LEN_PER_DATA_FRAME 50

typedef struct sample_
{
	int16_t v;
	int16_t i;
} sample;

extern sample g_adcb0[], g_adcb1[];

void samples_start();
void samples_stop();
void samples_end_calibration();
void samples_ovs(sample* s);
void samples_ringbuf_write(sample* s);
int samples_uart_write(uint8_t just_prepare);
int samples_store_write();
void samples_store_read_uart_frame(uint16_t file_num, uint32_t uart_frame);

#endif
