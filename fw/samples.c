#include "common.h"

#include "params.h"
#include "control.h"
#include "ringbuf.h"
#include "error.h"
#include "fs.h"

#include "samples.h"

sample g_adcb0[SAMPLES_LEN], g_adcb1[SAMPLES_LEN];
uint8_t g_samples_calibrated;

static ringbuf rb;
static uint8_t samples_tmp[sizeof(uint32_t) + sizeof(uint16_t) +
	(SAMPLES_LEN * sizeof(sample))];

static uint32_t uart_seqnum;

static void samples_init() //{{{
{
	ringbuf_init(&rb, 0x0000, SRAM_SIZE_BYTES,
		sram_write, sram_read);
	uart_seqnum = 0;
	g_samples_calibrated = 0;
} //}}}

void samples_start() //{{{
{
	// init sampling state
	samples_init();

	// open file if in storage mode
	if (g_control_mode == CONTROL_MODE_STORE)
	{
		if (fs_open(1) < 0)
			halt(ERROR_FS_OPEN_FAIL);
	}

	// set state to just started run
	g_samples_calibrated = 0;
	uart_seqnum = 0;

	// set sample rate
	params_set_samplerate();

	// current amp to minimum gain
	params_set_gain(PARAM_GAIN_CAL);
	// current measurement input to gnd
	mux_select(MUX_GND);
	// voltage measurement input to gnd
	ADCB.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN7_gc | ADC_CH_MUXNEG_GND_MODE4_gc;
	// wait for things to settle
	timer_sleep_ms(10);

	// start getting samples from the ADCs
	dma_start(g_adcb0, g_adcb1, SAMPLES_LEN*sizeof(sample));
} //}}}

void samples_stop() //{{{
{
	dma_stop();

	// close file if in storage mode
	if (g_control_mode == CONTROL_MODE_STORE)
	{
		if (fs_close() < 0)
			halt(ERROR_FS_CLOSE_FAIL);
	}
} //}}}

void samples_end_calibration() //{{{
{
	dma_pause(1);		
	// voltage measurment
	ADCB.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN0_gc | ADC_CH_MUXNEG_GND_MODE4_gc; 
	// current measurement
	params_set_gain(g_control_gain);
	mux_select(MUX_R);

	g_samples_calibrated = 1;
	dma_pause(0);		
} //}}}

void samples_ringbuf_write(sample* s) //{{{
{
	int ret;
	uint16_t len_b = SAMPLES_LEN * sizeof(sample);

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

int samples_uart_write() // {{{
{
	uint16_t len = 0;
	uint16_t samples_len = (SAMPLES_LEN * sizeof(sample));

	// abort if UART TX is not ready
	if (!uart_tx_ready())
		return -1;

	// sequence number
	memcpy(samples_tmp + len,
		&uart_seqnum,
		sizeof(uint32_t));
	len += sizeof(uint32_t);

	// length (bytes)
	memcpy(samples_tmp + len,
		&samples_len,
		sizeof(uint16_t));
	len += sizeof(uint16_t);

	// samples
	if (ringbuf_read(&rb, samples_tmp + len, samples_len) < 0)
		return -2; // no samples ready yet
	len += samples_len;

	uart_tx_bytes_dma(UART_TYPE_SAMPLES, samples_tmp, len);	

	// completed tx, advance to next seqnum
	uart_seqnum++;

	return 0;
} //}}}

void samples_store_write() //{{{
{
	uint16_t samples_len = (SAMPLES_LEN * sizeof(sample));

	// a write is in progress, continue it
	if (fs_busy())
		fs_update();
	// no write in progress, try to write samples to the filesystem
	else
	{
		if (ringbuf_read(&rb, samples_tmp, samples_len) < 0)
			return; // no samples ready yet

		if (fs_write(samples_tmp, samples_len) < 0)
			halt(ERROR_FS_WRITE_FAIL);
	}
} //}}}

static void uart_write_end() //{{{
{
	uint16_t len = 0;
	uint16_t samples_len = 0;

	// wait for any existing UART TX to complete
	while (!uart_tx_ready());

	// sequence number
	memcpy(samples_tmp + len,
		&uart_seqnum,
		sizeof(uint32_t));
	len += sizeof(uint32_t);

	// length (bytes)
	memcpy(samples_tmp + len,
		&samples_len,
		sizeof(uint16_t));
	len += sizeof(uint16_t);

	uart_tx_bytes_dma(UART_TYPE_SAMPLES, samples_tmp, len);	

	// wait for final UART TX to complete
	while (!uart_tx_ready());
} //}}}

void samples_store_read_uart_write() //{{{
{
	sample sd_samples[SAMPLES_LEN];

	// init sampling state
	samples_init();

	// open last file
	if (fs_open(0) < 0)
		halt(ERROR_FS_OPEN_FAIL);

	while(fs_read(sd_samples, sizeof(sd_samples)) >= 0)
	{
		samples_ringbuf_write(sd_samples, SAMPLES_LEN);
		// TODO if killed within write, this will loop forever
		samples_uart_write();
		// must wait for UART tx, SD read shares DMA channel
		while (!uart_tx_ready());
	}

	uart_write_end();
} //}}}
