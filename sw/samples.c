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
	float v_adcv = (((float)(s->signal)) / ADC_TOP) * VREF;
	return (v_adcv / V_DEV) * 1000.0; // undo the voltage divider
} //}}}

float sample_i(sample* s, float gain, float current_offset) //{{{
{
	// current
	if (s->signal == ADC_MAX)
		fprintf(stderr, "warning: maximum current, won't hurt anything, but you should turn down the gain.\n");

	if (s->signal < 0)
		s->signal = 0;
	float i_adcv = (((float)(s->signal)) / ADC_TOP) * VREF;
	float i_adcv_unamp = i_adcv / gain; // undo the current gain
	float i_samp = ((i_adcv_unamp / IRES_OHM) * 1000.0) - current_offset;
	if (i_samp < 0)
		i_samp = 0;
	return i_samp;
} //}}}

uint16_t samples_read(sample* v_s, sample* i_s, uint32_t* seqnum) //{{{
{
	uint8_t c1 = 0, c2 = 0;
	samples_hdr hdr;

	while (c1 == 0 || c2 == 0)
	{
		uart_read(&c1, 1);
		if (c1 == SAMPLES_DELIM0)
		{
			uart_read(&c2, 1);
			if (c2 == SAMPLES_DELIM1)
			{
				uart_read(&hdr, sizeof(hdr));

				if (*seqnum != 0)
				{
					if (hdr.seqnum != *seqnum + 1)
						fprintf(stderr, "error: expected sample frame seqnum %d got seqnum %d\n", *seqnum + 1, hdr.seqnum);
				}
				*seqnum = hdr.seqnum;

				// EOF for file read
				if (hdr.samples_len == 0)
					break;

				uart_read(v_s, hdr.samples_len);
				uart_read(i_s, hdr.samples_len);
			}
			else
			{
				c2 = 0;
			}
		}
		else
		{
			c1 = 0;
		}
	}

	return hdr.samples_len / sizeof(sample);
} //}}}

void samples_print_loop(float gain, float current_offset, char verb) //{{{
{
	int i;
	sample v_s[2000], i_s[2000];
	uint32_t seqnum = 1;
	uint16_t samples_len;
	int32_t v_cal = 0, i_cal = 0;

	// read calibration and compute it
	while (seqnum != 0)
	{
		samples_len = samples_read(v_s, i_s, &seqnum);
	}

	for (i = 0; i < samples_len; i++)
	{
		v_cal += v_s[i].signal;
		i_cal += i_s[i].signal;
	}
	v_cal = v_cal / samples_len;
	i_cal = i_cal / samples_len;
	if (verb)
	{
		printf("v_cal %d, i_cal %d\n", v_cal, i_cal);	
	}

	// main sample read loop
	while(1)
	{
		if ((samples_len = samples_read(v_s, i_s, &seqnum)) == 0)
			return;

		for (i = 0; i < samples_len; i++)
		{
			i_s[i].signal -= i_cal;
			v_s[i].signal -= v_cal;
			if (verb)
			{
				printf("i %d\n", i_s[i].signal);
				printf("v %d\n", v_s[i].signal);
			}
			float mv = sample_v(v_s+i);
			float mi = sample_i(i_s+i, gain, current_offset);
			printf("%f %f\n", mi, mv);
		}
	}
}
