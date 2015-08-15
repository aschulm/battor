#include "common.h"

#include "store.h"
#include "samples.h"
#include "error.h"

uint32_t g_samples_uart_seqnum;
sample g_adcb0[SAMPLES_LEN], g_adcb1[SAMPLES_LEN];

void samples_uart_write(sample* s, uint16_t len) //{{{
{
	uint16_t len_b = len*sizeof(sample);

	uart_tx_start(UART_TYPE_SAMPLES);

	// write seqnum
	uart_tx_bytes(&g_samples_uart_seqnum, sizeof(g_samples_uart_seqnum));

	// write number of samples
	uart_tx_bytes(&len_b, sizeof(len_b));

#ifdef SAMPLE_ZERO
  memset(s, 0, len_b);
#endif

#ifdef SAMPLE_INC
	int i;
	for (i = 0; i < len; i++)
	{
		s[i].v = i;
		s[i].i = i;
	}
#endif

	// write samples
	uart_tx_bytes(s, len_b);

	uart_tx_end();

	g_samples_uart_seqnum++;
} //}}}

void samples_store_write(sample* s) //{{{
{
	uint16_t len = SAMPLES_LEN * sizeof(sample);

	printf("samples: store_write_bytes\r\n");
	// write samples
	store_write_bytes((uint8_t*)s, len);
} //}}}

uint16_t samples_store_read_next(sample* s) //{{{
{
	uint16_t len = SAMPLES_LEN * sizeof(sample);
	int ret = 0;

	// read samples
	ret = store_read_bytes((uint8_t*)s, len);
	return (ret >= 0) ? len : 0;
} //}}}
