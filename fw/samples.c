#include "common.h"

#include "store.h"
#include "samples.h"
#include "error.h"

uint8_t g_samples_uart_seqnum;
uint16_t g_samples_read_file;
uint8_t g_adca0[SAMPLES_LEN], g_adca1[SAMPLES_LEN], g_adcb0[SAMPLES_LEN], g_adcb1[SAMPLES_LEN];

void samples_uart_write(uint8_t* v_s, uint8_t* i_s, uint16_t len) //{{{
{
	// write frame delimiter
	uart_tx_byte(SAMPLES_DELIM0);
	uart_tx_byte(SAMPLES_DELIM1);

	// write seqnum
	uart_tx_byte(g_samples_uart_seqnum);

	// write number of samples
	uart_tx_bytes(&len, sizeof(len)); 

	// write voltage samples
	uart_tx_bytes(v_s, len);
	// write current samples
	uart_tx_bytes(i_s, len);

	g_samples_uart_seqnum++;

	memset(v_s, 0, len);
	memset(i_s, 0, len);
} //}}}

void samples_store_write(uint8_t* v_s, uint8_t* i_s) //{{{
{
	uint16_t len = SAMPLES_LEN;

	printf("samples: store_write_bytes\r\n");
	// write voltage samples
	store_write_bytes(v_s, len);
	// write current samples
	store_write_bytes(i_s, len);
} //}}}

void samples_store_read(uint16_t file) //{{{
{
	uint16_t len = SAMPLES_LEN;
	int ret_v = 0, ret_i = 0;

	// open file
	store_read_open(file);

	// read voltage
	ret_v = store_read_bytes(g_adca0, len);
	// read current
	ret_i = store_read_bytes(g_adcb0, len);
	while (ret_v >= 0 && ret_i >= 0)
	{
		led_toggle(LED_GREEN_bm);
		samples_uart_write(g_adca0, g_adcb0, SAMPLES_LEN);
		// read voltage
		ret_v = store_read_bytes(g_adca0, len);
		// read current
		ret_i = store_read_bytes(g_adcb0, len);
	}
	samples_uart_write(NULL, NULL, 0); // no more samples

} //}}}
