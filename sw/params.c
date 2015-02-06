#include "common.h"

uint32_t param_sample_rate(uint32_t desired_sample_rate_hz, uint16_t ovs_bits, uint16_t* t_ovf, uint16_t* t_div, uint16_t* filpot_pos) //{{{
{
	verb_printf("param_sample_rate: desired_sample_rate_hz %d before oversampling\n",
		desired_sample_rate_hz);
	// up the desired sample rate based on the desired oversampling
	if (ovs_bits > 0)
	{
		desired_sample_rate_hz *= (4 << ((ovs_bits-1)*2));
	}
	verb_printf("param_sample_rate: desired_sample_rate_hz %d after oversampling\n",
		desired_sample_rate_hz);

	// for now just loop through every 64div overflow and see what is closest
	uint32_t actual_sample_rate_hz = 0;
	uint32_t clock_hz = CLOCK_HZ / 8;
	uint32_t ovf = clock_hz / desired_sample_rate_hz; 

	verb_printf("param_sample_rate: overflow %d = clock %d / desired_sample_rate_hz %d\n",
		ovf, clock_hz, desired_sample_rate_hz);

	// 16 bit timer overflow
	if (ovf > 0xFFFF)
	{
		fprintf(stderr, "warning: sample rate out of range, reverting to default\n");
		ovf = TIMER_OVF_DEFAULT;
	}

	actual_sample_rate_hz = clock_hz / ovf;
	if (ovs_bits > 0)
	{
		actual_sample_rate_hz = actual_sample_rate_hz / (4 << ((ovs_bits-1)*2));
	}

	// set battor timer and current filter parameters
	*t_div = XMEGA_CLKDIV_8_gc;
	*t_ovf = (ovf-1); // -1 is extremely important! this is for overflow

	double filpot_r = 1.0 / ((desired_sample_rate_hz/2.0)*2.0*M_PI*FILCAP_F);
	*filpot_pos = (filpot_r / FILPOT_OHM) * 1024;

	return actual_sample_rate_hz;
} //}}}

double param_gain(uint32_t desired_gain, uint16_t* amppot_pos) //{{{
{
	if (desired_gain <= 1)
	{
			fprintf(stderr, "warning: gain less than 2, reverting to default\n");
		*amppot_pos = AMPPOT_POS_DEFAULT;
	}

	*amppot_pos = (uint16_t)(((100000.0/(desired_gain - 1.0)) / AMPPOT_OHM) * 1024.0);

	if (*amppot_pos == 0 || *amppot_pos > 1023)
	{
		fprintf(stderr, "warning: gain out of range, reverting to default\n");
		*amppot_pos = AMPPOT_POS_DEFAULT;
	}
	return (100000.0 / ((((double)*amppot_pos)/1024.0) * AMPPOT_OHM)) + 1.0;
} //}}}
