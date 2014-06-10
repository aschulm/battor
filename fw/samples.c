#include "common.h"

#include "store.h"
#include "samples.h"
#include "error.h"

uint8_t g_samples_uart_seqnum;

void samples_uart_write(uint8_t* v_s, uint8_t* i_s) //{{{
{
	uint16_t len = SAMPLES_LEN;

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

static uint8_t store_v_s[SAMPLES_LEN], store_i_s[SAMPLES_LEN];
void samples_store_read(uint16_t file) //{{{
{
	uint16_t len = SAMPLES_LEN;
	int ret_v = 0, ret_i = 0;

	// open file
	store_read_open(file);

	// read voltage
	ret_v = store_read_bytes(store_v_s, len);
	// read current
	ret_i = store_read_bytes(store_i_s, len);

	printf("samples: store_read v %d i %d\r\n", ret_v, ret_i);

	while (ret_v >= 0 && ret_i >= 0)
	{
		led_toggle(LED_GREEN_bm);
		samples_uart_write(store_v_s, store_i_s);
		// read voltage
		ret_v = store_read_bytes(store_v_s, len);
		// read current
		ret_i = store_read_bytes(store_i_s, len);
	}
} //}}}
