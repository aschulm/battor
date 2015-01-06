#include "common.h"
#include "uart.h"
#include "params.h"
#include "samples.h"

static float s_adc_top;

void samples_init(uint16_t ovs_bits) //{{{
{
	// determine ADC_TOP with ovsersampling
	s_adc_top = pow(2, (ADC_BITS + ovs_bits));
	verb_printf("adc_top %f\n", s_adc_top);
} //}}}

float sample_v(sample* s) //{{{
{
	if (s->signal == (s_adc_top-1))
		fprintf(stderr, "warning: maximum voltage, won't hurt anything, but what phone battery has such a high voltage?\n");
	if (s->signal < 0)
		s->signal = 0;
	float v_adcv = (((float)(s->signal)) / s_adc_top) * VREF;
	return (v_adcv / V_DEV) * 1000.0; // undo the voltage divider
} //}}}

float sample_i(sample* s, float gain, float current_offset) //{{{
{
	// current
	if (s->signal == (s_adc_top-1))
		fprintf(stderr, "warning: maximum current, won't hurt anything, but you should turn down the gain.\n");

	if (s->signal < 0)
		s->signal = 0;
	float i_adcv = (((float)(s->signal)) / s_adc_top) * VREF;
	float i_adcv_unamp = i_adcv / gain; // undo the current gain
	float i_samp = ((i_adcv_unamp / IRES_OHM) * 1000.0) - current_offset;
	if (i_samp < 0)
		i_samp = 0;
	return i_samp;
} //}}}

uint16_t samples_read(sample* v_s, sample* i_s, uint32_t* seqnum) //{{{
{
	samples_hdr* hdr;
	uint8_t frame[50000];
	uint8_t* frame_ptr;
	uint8_t type;

	while (uart_rx_bytes(&type, frame, sizeof(frame)) < 0);
	frame_ptr = frame;

	hdr = (samples_hdr*)frame_ptr;
	frame_ptr += sizeof(samples_hdr);

	verb_printf("samples_read: len:%d seqnum:%d\n", hdr->samples_len, hdr->seqnum);

	if (*seqnum != 0)
	{
		if (hdr->seqnum != *seqnum + 1)
			fprintf(stderr, "error: expected sample frame seqnum %d got seqnum %d\n", *seqnum + 1, hdr->seqnum);
	}
	*seqnum = hdr->seqnum;

	memcpy(v_s, frame_ptr, hdr->samples_len);
	frame_ptr += hdr->samples_len;

	memcpy(i_s, frame_ptr, hdr->samples_len);
	frame_ptr += hdr->samples_len;

	return hdr->samples_len / sizeof(sample);
} //}}}

void samples_print_loop(float gain, float current_offset, float ovs_bits, char verb) //{{{
{
	int i;
	sample v_s[2000], i_s[2000];
	uint32_t seqnum = 0;
	uint16_t samples_len;
	int32_t v_cal = 0, i_cal = 0;
	sigset_t sigs;

	// will block and unblock SIGINT
	sigemptyset(&sigs);
	sigaddset(&sigs, SIGINT);

	// read calibration and compute it
	while (seqnum != 1)
	{
		samples_len = samples_read(v_s, i_s, &seqnum);
	}

	for (i = 0; i < samples_len; i++)
	{
		verb_printf("i_cal %d\n", i_s[i].signal);
		verb_printf("v_cal %d\n", v_s[i].signal);

		v_cal += v_s[i].signal;
		i_cal += i_s[i].signal;
	}
	v_cal = v_cal / samples_len;
	i_cal = i_cal / samples_len;
	verb_printf("v_cal_avg %d, i_cal_avg %d\n", v_cal, i_cal);	

	// main sample read loop
	while(1)
	{
		if ((samples_len = samples_read(v_s, i_s, &seqnum)) == 0)
			return;

		for (i = 0; i < samples_len; i++)
		{
			verb_printf("i %d\n", i_s[i].signal);
			verb_printf("v %d\n", v_s[i].signal);

			i_s[i].signal -= i_cal;
			v_s[i].signal -= v_cal;

			float mv = sample_v(v_s+i);
			float mi = sample_i(i_s+i, gain, current_offset);

			sigprocmask(SIG_BLOCK, &sigs, NULL);   // disable interrupts before print
			printf("%f %f\n", mi, mv);
			sigprocmask(SIG_UNBLOCK, &sigs, NULL); // enable interrupts before print
		}
	}
}
