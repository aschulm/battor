#include "common.h"

#include "store.h"
#include "samples.h"
#include "error.h"

uint32_t g_samples_ovsamp_bits = 0;
uint32_t g_samples_uart_seqnum;
uint16_t g_samples_read_file;
int16_t g_adca0[SAMPLES_LEN], g_adca1[SAMPLES_LEN], g_adcb0[SAMPLES_LEN], g_adcb1[SAMPLES_LEN];

// TODO for some reason at g_samples_ovsamp_bits 3 the first adc sample from the voltage is sometimes -something
uint16_t samples_ovsample(int16_t* v_s, int16_t* i_s, uint16_t len) //{{{
{
	uint8_t s_in_idx = 0;
	uint8_t s_out_idx = 0;

	uint8_t s_ovs_len = (4 << ((g_samples_ovsamp_bits-1) << 1));

	uint16_t s_ovs_idx_end = 0;

	int16_t v_sum = 0;
	int16_t i_sum = 0;

	// no oversampling
	if (g_samples_ovsamp_bits <= 0)
		return len;

	// oversample
	while (s_in_idx < len)
	{
		v_sum = 0;
		i_sum = 0;

		s_ovs_idx_end = s_in_idx + s_ovs_len;

		while (s_in_idx < s_ovs_idx_end)
		{
			// add samples to sums (unrolled for performance)
			v_sum += v_s[s_in_idx];
			i_sum += i_s[s_in_idx++];
			v_sum += v_s[s_in_idx];
			i_sum += i_s[s_in_idx++];
			v_sum += v_s[s_in_idx];
			i_sum += i_s[s_in_idx++];
			v_sum += v_s[s_in_idx];
			i_sum += i_s[s_in_idx++];
		}
		
		// store sums
		v_s[s_out_idx] = (v_sum >> g_samples_ovsamp_bits);
		i_s[s_out_idx++] = (i_sum >> g_samples_ovsamp_bits);
	}

	return len / s_ovs_len;
} //}}}

void samples_uart_write(int16_t* v_s, int16_t* i_s, uint16_t len) //{{{
{
	len *= sizeof(int16_t);

	uart_tx_start(UART_TYPE_SAMPLES);

	// write seqnum
	uart_tx_bytes(&g_samples_uart_seqnum, sizeof(g_samples_uart_seqnum));

	// write number of samples
	uart_tx_bytes(&len, sizeof(len)); 

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

void samples_store_read(uint16_t file) //{{{
{
	uint16_t len = SAMPLES_LEN * sizeof(int16_t);
	int ret_v = 0, ret_i = 0;

	// open file
	store_read_open(file);

	// read voltage
	ret_v = store_read_bytes((uint8_t*)g_adca0, len);
	// read current
	ret_i = store_read_bytes((uint8_t*)g_adcb0, len);
	while (ret_v >= 0 && ret_i >= 0)
	{
		samples_uart_write(g_adca0, g_adcb0, SAMPLES_LEN);
		// read voltage
		ret_v = store_read_bytes((uint8_t*)g_adca0, len);
		// read current
		ret_i = store_read_bytes((uint8_t*)g_adcb0, len);
	}
	samples_uart_write(NULL, NULL, 0); // no more samples

} //}}}
