#include "common.h"

#include "store.h"
#include "samples.h"
#include "ringbuf.h"
#include "error.h"

uint32_t g_samples_uart_seqnum;
sample g_adcb0[SAMPLES_LEN], g_adcb1[SAMPLES_LEN];
static ringbuf rb;
static uint8_t samples_tmp[400];

void samples_init()
{
	ringbuf_init(&rb, 0x0000, SRAM_SIZE_BYTES,
		sram_write, sram_read);
}

void samples_ringbuf_write(sample* s, uint16_t len) //{{{
{
	int ret;
	uint16_t len_b = len * sizeof(sample);

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
	ret = ringbuf_write(&rb, s, len_b);
	if (ret < 0)
		halt(ERROR_RINGBUF_WRITE_FAIL);
} //}}}

void samples_uart_write() // {{{
{
	uint16_t len = 0;
	uint16_t samples_len = 20 * sizeof(sample);
	
	// sequence number
	memcpy(samples_tmp + len,
		&g_samples_uart_seqnum,
		sizeof(uint32_t));
	len += sizeof(uint32_t);

	// length (bytes)
	memcpy(samples_tmp + len,
		&samples_len,
		sizeof(uint16_t));
	len += sizeof(uint16_t);

	// samples
	if (ringbuf_read(&rb, samples_tmp + len, samples_len) < 0)
		return; // no samples ready yet
	len += samples_len;

	printf("ringbuf remaining %d\n", rb.len);

	uart_tx_bytes_dma(UART_TYPE_SAMPLES, samples_tmp, len);	

	// completed tx, advance to next seqnum
	g_samples_uart_seqnum++;
}

void samples_store_write(sample* s) //{{{
{
	uint16_t len = SAMPLES_LEN * sizeof(sample);

	printf("samples: store_write_bytes\n");
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
