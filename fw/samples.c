#include "common.h"

#include "params.h"
#include "control.h"
#include "ringbuf.h"
#include "error.h"
#include "fs.h"

#include "samples.h"

sample g_adcb0[SAMPLES_LEN], g_adcb1[SAMPLES_LEN];

static ringbuf rb;
static uint8_t samples_tmp[sizeof(uint32_t) + sizeof(uint16_t) +
	(SAMPLES_LEN * sizeof(sample))];
static uint8_t uart_tx_buffer[UART_TX_BUFFER_LEN];

static uint8_t state;
static uint8_t calibrated;
static uint32_t seqnum;
static int16_t ringbuf_writes_remaining;
static int16_t ringbuf_reads_remaining;
static uint16_t samples_len;

typedef enum STATE_enum
{
	STATE_PRE_FRAME = 0,
	STATE_MID_FRAME,
	STATE_END_FRAME,
} STATE_t;

static void end_calibration() //{{{
{
	// voltage measurment
	ADCB.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN0_gc | ADC_CH_MUXNEG_GND_MODE4_gc; 
	// current measurement
	ADCB.CH1.MUXCTRL = ADC_CH_MUXPOS_PIN1_gc | ADC_CH_MUXNEG_GND_MODE3_gc;

	calibrated = 1;
} ////}}}

static void end_uart_write() //{{{
{
	uint16_t len = 0;

	// zero sample length indicates end of download
	uint16_t samples_len = 0;

	// sequence number
	memcpy(samples_tmp + len, &seqnum,
		sizeof(uint32_t));
	len += sizeof(uint32_t);

	// length (bytes)
	memcpy(samples_tmp + len,
		&samples_len,
		sizeof(uint16_t));
	len += sizeof(uint16_t);

	uart_tx_start_prepare(UART_TYPE_SAMPLES);
	uart_tx_bytes_prepare(samples_tmp, len);
	uart_tx_end_prepare();
	uart_tx();
} //}}}

static void samples_init() //{{{
{
	uint8_t port_ovs_bits;

	ringbuf_init(&rb, 0x00000000, SRAM_SIZE_BYTES,
		sram_write, sram_read);

	state = STATE_PRE_FRAME;
	ringbuf_writes_remaining = SAMPLES_LEN_PER_CAL_FRAME;
	ringbuf_reads_remaining = SAMPLES_LEN_PER_CAL_FRAME;
	calibrated = 0;
	seqnum = 0;

	// start samples length with default
	samples_len = (SAMPLES_LEN * sizeof(sample));
	// divide samples length by number of averaged samples
	if ((g_control_mode == CONTROL_MODE_PORT_IDLE ||
		g_control_mode == CONTROL_MODE_PORT_STORE) &&
		(port_ovs_bits = params_get_port_ovs_bits()) > 0)
		samples_len = (samples_len >> (port_ovs_bits*2));
} //}}}

void samples_start() //{{{
{
	int ret;
	// init sampling state
	samples_init();

	// open file if in storage mode
	if (g_control_mode == CONTROL_MODE_USB_STORE ||
		g_control_mode == CONTROL_MODE_PORT_STORE)
	{
		if ((ret = fs_open(1, 0)) < 0)
			halt(ERROR_FS_OPEN_FAIL);
	}

	// set sample rate
	params_set_samplerate();

	// current amp to control gain value
	params_set_gain(g_control_gain);
	// voltage measurement input to gnd
	ADCB.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN7_gc | ADC_CH_MUXNEG_GND_MODE4_gc;
	// current measurement input to gnd
	ADCB.CH1.MUXCTRL = ADC_CH_MUXPOS_PIN7_gc | ADC_CH_MUXNEG_GND_MODE3_gc;
	mux_select(MUX_R);
	// wait for things to settle
	timer_sleep_ms(10);

	// start getting samples from the ADCs
	dma_start(g_adcb0, g_adcb1);
	// set the rtc start timestamp for the file when in portable mode
	if (g_control_mode == CONTROL_MODE_PORT_STORE)
		fs_rtc_set();
} //}}}

void samples_stop() //{{{
{
	dma_stop();

	// close file if in storage mode
	if (g_control_mode == CONTROL_MODE_USB_STORE ||
		g_control_mode == CONTROL_MODE_PORT_STORE)
	{
		// read to the end of the ring buffer
		while (samples_store_write());

		if (fs_close() < 0)
			halt(ERROR_FS_CLOSE_FAIL);
	}
} //}}}

void samples_ovs(sample* s) //{{{
{
	uint8_t port_ovs_bits;
	if (g_control_mode != CONTROL_MODE_PORT_STORE ||
		(port_ovs_bits = params_get_port_ovs_bits()) <= 0)
		return;

	// loop through all samples and oversample
	uint16_t idx, idx_avg;
	for (idx = 0; idx < samples_len; idx++)
	{
		int32_t i_avg = 0, v_avg = 0;
		for (idx_avg = 0; idx_avg < (1<<(port_ovs_bits*2)); idx_avg++)
		{
			i_avg += s[(idx<<(port_ovs_bits*2)) + idx_avg].i;
			v_avg += s[(idx<<(port_ovs_bits*2)) + idx_avg].v;
		}
		s[idx].i = (i_avg >> port_ovs_bits);
		s[idx].v = (v_avg >> port_ovs_bits);
	}
} //}}}

void samples_ringbuf_write(sample* s) //{{{
{
	// check to see if enough samples collected to complete calibration
	ringbuf_writes_remaining--;
	if (!calibrated && ringbuf_writes_remaining == 0)
		end_calibration();

#ifdef SAMPLE_ZERO
	memset(s, 0, samples_len);
#endif

#ifdef SAMPLE_INC
	int i;
	for (i = 0; i < SAMPLES_LEN; i++)
	{
		s[i].v = i;
		s[i].i = i;
	}
#endif

	// write samples
	if (ringbuf_write(&rb, s, samples_len) < 0)
	{
		// the only known cause now for ringbuffer being full would be
		// that the sd card can't keep up so notify the sd card driver
		// we're debugging and want to see how long it's been since the
		// last successful block write.
		sd_overrun();
		halt(ERROR_RINGBUF_WRITE_FAIL);
	}
} //}}}

int samples_uart_write(uint8_t just_prepare) // {{{
{
	uint16_t len = 0;
	uint8_t uart_buf[20];

	if (!just_prepare)
	{
		// abort if UART TX is not ready
		if (!uart_tx_ready())
			return -1;
	}

	// have not started a writing a frame -- write frame header
	if (rb.len && state == STATE_PRE_FRAME)
	{
		len = 0;

		// compute length of samples in frame in bytes
		uint16_t samples_len_hdr = samples_len *
			ringbuf_reads_remaining;

		// sequence number
		memcpy(uart_buf + len, &seqnum,
			sizeof(uint32_t));
		len += sizeof(uint32_t);

		// length (bytes)
		memcpy(uart_buf + len,
			&samples_len_hdr,
			sizeof(uint16_t));
		len += sizeof(uint16_t);

		uart_tx_start_prepare(UART_TYPE_SAMPLES);
		uart_tx_bytes_prepare(uart_buf, len);
		if (!just_prepare)
			uart_tx();

		// advance state to mid frame
		state = STATE_MID_FRAME;
	}

	// in the middle of a frame -- write frame samples
	else if (state == STATE_MID_FRAME)
	{
		len = 0;

		// fetch samples from ringbuf
		if (ringbuf_read(&rb, samples_tmp, samples_len) < 0)
			return -2; // no samples ready yet
		len += samples_len;
		ringbuf_reads_remaining--;

		uart_tx_bytes_prepare(samples_tmp, len);
		if (!just_prepare)
			uart_tx_dma();

		// advance state to end frame
		if (ringbuf_reads_remaining == 0)
			state = STATE_END_FRAME;
	}

	// at the end of a frame -- write end an start next frame
	else if (state == STATE_END_FRAME)
	{

		// write last byte of the frame
		uart_tx_end_prepare();
		if (!just_prepare)
			uart_tx();

		// increment frame seqnum
		seqnum++;

		// prepare to write next frame
		state = STATE_PRE_FRAME;
		ringbuf_reads_remaining = SAMPLES_LEN_PER_DATA_FRAME;
		ringbuf_writes_remaining = SAMPLES_LEN_PER_DATA_FRAME;
	}

	// at the end of the sample stream has been reached
	else
		return -3;

	return 0;
} //}}}

int samples_store_write() //{{{
{
	uint8_t* uart_tx_buffer;
	int16_t reads_remaining_prev = -1;

	// SD shares a DMA channel with UART - abort if UART is in use
	if (!dma_uart_tx_ready())
		return 1;

	// a write is in progress, continue it
	if (fs_busy())
		fs_update();

	// no write in progress, try to write frame to the filesystem
	else
	{
		/*
		 * This is very odd. The goal here is to
		 * put the CPU into the same mode that it is in the
		 * UART streaming mode so they get the same measurements.
		 */

		// wait until a sample block is written to samples_tmp
		reads_remaining_prev = ringbuf_reads_remaining;
		while (samples_uart_write(1) >= 0 &&
		 ringbuf_reads_remaining >=	reads_remaining_prev)
		{
			reads_remaining_prev = ringbuf_reads_remaining;
			uart_get_tx_buffer(&uart_tx_buffer);
		}
		// clear bytes leftover from last write
		uart_get_tx_buffer(&uart_tx_buffer);

		// if there was a sample block written, write it to sd
		if (ringbuf_reads_remaining < reads_remaining_prev)
		{
			if (fs_write(samples_tmp, samples_len) < 0)
				halt(ERROR_FS_WRITE_FAIL);
		}
		else
			return 0;
	}

	return 1;
} //}}}

void samples_store_read_uart_frame(uint16_t file_num, uint32_t seqnum_req) //{{{
{
	uint32_t tries;
	uint16_t len;
	sample sd_samples[SAMPLES_LEN];
	uint8_t* uart_driver_tx_buffer;
	uint8_t* tx_buffer_a;
	uint8_t* tx_buffer_b;
	int32_t file_byte_len;
	uint32_t uart_frame_byte_idx;

	// wait until any outstanding UART transactions are done
	tries = 0;
	while(!dma_uart_tx_ready() &&
		tries++ < SAMPLES_UART_TX_TIMEOUT_20US)
		_delay_us(20);
	// timeout waiting for uart tx to complete
	if (tries >= SAMPLES_UART_TX_TIMEOUT_20US)
		goto cleanup;

	// switch DMA into UART and SPI only mode because 3 channels are needed
	dma_init(1);

	// init sampling state.
	samples_init();

	// remember the uart driver's default buffer
	uart_get_tx_buffer(&uart_driver_tx_buffer);

	// setup double_buffering
	tx_buffer_a = uart_driver_tx_buffer;
	tx_buffer_b = uart_tx_buffer;

	// open file
	if (fs_open(0, file_num) < 0)
	{
		// error: no file found, indicate as such by writing a blank end of file
		end_uart_write();
		goto cleanup;
	}

	// get file size
	if ((file_byte_len = fs_size_get()) < 0)
	{
		// error: no file found, indicate by writing a end of file
		end_uart_write();
		goto cleanup;
	}

	// prepare the UART frame generator state
	state = STATE_PRE_FRAME;
	seqnum = seqnum_req;

	// prepare to start reading requested UART frame of calibration samples
	if (seqnum_req == 0)
	{
		uart_frame_byte_idx = 0;

		int32_t samples_len_remaining = (file_byte_len - uart_frame_byte_idx) / samples_len;

		// error: Not enough calibration samples, indicate by writing a end of file
		if (samples_len_remaining < SAMPLES_LEN_PER_CAL_FRAME)
		{
			end_uart_write();
			goto cleanup;
		}

		ringbuf_reads_remaining = SAMPLES_LEN_PER_CAL_FRAME;
		ringbuf_writes_remaining = SAMPLES_LEN_PER_CAL_FRAME;
	}
	// prepare to start reading requested UART frame of data samples
	else
	{
		uart_frame_byte_idx = (samples_len * SAMPLES_LEN_PER_CAL_FRAME) +
			((seqnum_req - 1) * samples_len * SAMPLES_LEN_PER_DATA_FRAME);

		int32_t samples_len_remaining = (file_byte_len - uart_frame_byte_idx) / samples_len;

		// end of file reached, write the last frame
		if (samples_len_remaining <= 0)
		{
			end_uart_write();
			goto cleanup;
		}
		// partial frame remaining
		else if (samples_len_remaining < SAMPLES_LEN_PER_DATA_FRAME)
		{
			ringbuf_reads_remaining = samples_len_remaining;
			ringbuf_writes_remaining = samples_len_remaining;
		}
		// full frame
		else
		{
			ringbuf_reads_remaining = SAMPLES_LEN_PER_DATA_FRAME;
			ringbuf_writes_remaining = SAMPLES_LEN_PER_DATA_FRAME;
		}
	}

	if (fs_seek(uart_frame_byte_idx) < 0)
	{
		// error: failed to seek, indicate by writing a end of file
		end_uart_write();
		goto cleanup;
	}

	// send the requested UART frame
	while (seqnum < (seqnum_req + 1))
	{
		uint8_t* tx_buffer_tmp;

		// abort if there is a pending control message
		if (uart_rx_is_pending())
			goto cleanup;

		// read next buffer of samples
		if (fs_read(sd_samples, samples_len) == FS_ERROR_EOF)
		{
			// With the prepare logic this should never happen, but if it does, exit gracefully.
			goto cleanup;
		}

		// write the samples to the ring buffer like during streaming operation
		samples_ringbuf_write(sd_samples);

		// write the samples to the uart buffer
		while (samples_uart_write(1) >= 0);

		// wait for uart transmission to complete
		tries = 0;
		while(!dma_uart_tx_ready() &&
			tries++ < SAMPLES_UART_TX_TIMEOUT_20US)
			_delay_us(20);

		// timeout waiting for uart tx to complete
		if (tries >= SAMPLES_UART_TX_TIMEOUT_20US)
			goto cleanup;

		// switch the uart driver to the other uart buffer
		len = uart_set_tx_buffer(tx_buffer_b);

		// transmit the uart buffer that was just filled
		dma_uart_tx(tx_buffer_a, len);

		// swap the buffers
		tx_buffer_tmp = tx_buffer_a;
		tx_buffer_a = tx_buffer_b;
		tx_buffer_b = tx_buffer_tmp;
	}

cleanup:
	// wait for uart transmission to complete
	tries = 0;
	while(!dma_uart_tx_ready() &&
			tries++ < SAMPLES_UART_TX_TIMEOUT_20US)
			_delay_us(20);

	// switch DMA back into normal mode and reset
	dma_init(0);

	// set the uart buffer back to the default
	uart_set_tx_buffer(uart_driver_tx_buffer);
} //}}}
