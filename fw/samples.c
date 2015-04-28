#include "common.h"

#include "store.h"
#include "samples.h"
#include "error.h"

uint32_t g_samples_uart_seqnum;
int16_t g_adca0[SAMPLES_LEN], g_adca1[SAMPLES_LEN], g_adcb0[SAMPLES_LEN], g_adcb1[SAMPLES_LEN];

void samples_uart_write(int16_t* v_s, int16_t* i_s, uint16_t len) //{{{
{
	len *= sizeof(int16_t);

	uart_tx_start(UART_TYPE_SAMPLES);

	// write seqnum
	uart_tx_bytes(&g_samples_uart_seqnum, sizeof(g_samples_uart_seqnum));

	// write number of samples
	uart_tx_bytes(&len, sizeof(len)); 

#ifdef SAMPLE_ZERO
  memset(v_s, 0, len);
  memset(i_s, 0, len);
#endif

#ifdef SAMPLE_INC
	int i;
	for (i = 0; i < len / sizeof(int16_t); i++)
	{
		v_s[i] = i;
		i_s[i] = i;
	}
#endif

	// write voltage samples
	uart_tx_bytes(v_s, len);
	// write current samples
	uart_tx_bytes(i_s, len);

	uart_tx_end();

	g_samples_uart_seqnum++;
} //}}}

void samples_store_write(int16_t* v_s, int16_t* i_s) //{{{
{
	uint16_t len = SAMPLES_LEN * sizeof(int16_t);

	printf("samples: store_write_bytes\r\n");
	// write voltage samples
	store_write_bytes((uint8_t*)v_s, len);
	// write current samples
	store_write_bytes((uint8_t*)i_s, len);
} //}}}

uint16_t samples_store_read_next(int16_t* v_s, int16_t* i_s) //{{{
{
	uint16_t len = SAMPLES_LEN * sizeof(int16_t);
	int ret_v = 0, ret_i = 0;

	// read voltage
	ret_v = store_read_bytes((uint8_t*)v_s, len);
	// read current
	ret_i = store_read_bytes((uint8_t*)i_s, len);
	return (ret_v >= 0 && ret_i >= 0) ? len : 0;
} //}}}
