#include <stdio.h>
#include <math.h>
#include <stdint.h>

#include "uart.h"
#include "params.h"
#include "samples.h"

float sample_v(sample* s) //{{{
{
	if (s->signal == ADC_MAX)
		fprintf(stderr, "warning: maximum voltage, won't hurt anything, but what phone battery has such a high voltage?\n");
	if (s->signal < 0)
		s->signal = 0;
	float v_adcv = ((float)(s->signal)) / ADC_TOP;
	return (v_adcv / V_DEV) * 1000.0; // undo the voltage divider
} //}}}

float sample_i(sample* s, float gain, float current_offset) //{{{
{
	// current
	if (s->signal == ADC_MAX)
		fprintf(stderr, "warning: maximum current, won't hurt anything, but you should turn down the gain.\n");

	if (s->signal < 0)
		s->signal = 0;
	float i_adcv = ((float)(s->signal)) / ADC_TOP;
	float i_adcv_unamp = i_adcv / gain; // undo the current gain
	float i_samp = ((i_adcv_unamp / IRES_OHM) * 1000.0) - current_offset;
	if (i_samp < 0)
		i_samp = 0;
	return i_samp;
} //}}}

void samples_print_loop(float gain, float current_offset) //{{{
{
	uint8_t c;
	int i;
	samples_hdr hdr;
	sample v_s[2000], i_s[2000];
	uint8_t prev_seqnum = 0;
	uint8_t next_seqnum = 0;

	while(1)
	{
		uart_read(&c, 1);
		if (c == SAMPLES_DELIM0)
		{
			uart_read(&c, 1);
			if (c == SAMPLES_DELIM1)
			{
				uart_read(&hdr, sizeof(hdr));

				if (prev_seqnum != 0 || next_seqnum != 0)
				{
					next_seqnum = prev_seqnum + 1;
					if (hdr.seqnum != next_seqnum)
						fprintf(stderr, "error: expected sample frame seqnum %d got seqnum %d\n", next_seqnum, hdr.seqnum);
				}
				prev_seqnum = hdr.seqnum;

				uart_read(v_s, hdr.samples_len);
				uart_read(i_s, hdr.samples_len);

				for (i = 0; i < (hdr.samples_len / sizeof(sample)); i++)
				{
					//printf("i %d\n", i_s[i].signal);
					//printf("v %d\n", v_s[i].signal);
					float mv = sample_v(v_s+i);
					float mi = sample_i(i_s+i, gain, current_offset);
					printf("%f %f\n", mi, mv);
				}
			}
		}
	}
}
