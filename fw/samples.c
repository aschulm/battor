#include "common.h"

#include "samples.h"
#include "ringbuf.h"
#include "error.h"

uint32_t g_samples_uart_seqnum;
sample g_adcb0[SAMPLES_LEN], g_adcb1[SAMPLES_LEN];
static ringbuf rb;
static uint8_t samples_tmp[sizeof(uint32_t) + sizeof(uint16_t) +
	(SAMPLES_LEN * sizeof(sample))];

static uint8_t sd_block_tmp[SD_BLOCK_LEN];
static uint32_t sd_block_idx;
static uint8_t sd_write_in_progress;

void samples_init() //{{{
{
	ringbuf_init(&rb, 0x0000, SRAM_SIZE_BYTES,
		sram_write, sram_read);
	g_samples_uart_seqnum = 0;

	sd_block_idx = 0;
	sd_write_in_progress = 0;
} //}}}

void samples_ringbuf_write(sample* s, uint16_t len) //{{{
{
	int ret;
	uint16_t len_b = len * sizeof(sample);

	printf("samples_ringbuf_write() len_b:%d\n", len_b); 

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
	uint16_t samples_len = (SAMPLES_LEN * sizeof(sample));

	// abort if UART TX is not ready
	if (!uart_tx_ready())
		return;

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

	uart_tx_bytes_dma(UART_TYPE_SAMPLES, samples_tmp, len);	

	// completed tx, advance to next seqnum
	g_samples_uart_seqnum++;
} //}}}

void samples_store_write() //{{{
{
	/*
	 * Write one SD card block worth of samples
	 */

	// if there is a write in progress, continue it
	if (sd_write_in_progress)
	{
		sd_write_in_progress = (sd_write_block_update() < 0);
	}

	// no write in progress, try to read a SD block of samples and write it
	else
	{
		if (ringbuf_read(&rb, sd_block_tmp, SD_BLOCK_LEN) < 0)
			return; // no samples ready yet

		// start SD samples write
		sd_write_block_start(sd_block_tmp, sd_block_idx++);
		sd_write_in_progress = 1;
	}
} //}}}

//uint16_t samples_store_read_next(sample* s) //{{{
//{
//	uint16_t len = SAMPLES_LEN * sizeof(sample);
//	int ret = 0;
//
//	// read samples
//	ret = store_read_bytes((uint8_t*)s, len);
//	return (ret >= 0) ? len : 0;
//} //}}}
