#include "common.h"

uint32_t param_sample_rate(uint32_t desired_sample_rate_hz, uint16_t* t_ovf, uint16_t* t_div, uint16_t* filpot_pos) //{{{
{
	// for now just loop through every 64div overflow and see what is closest
	uint32_t clock_hz = CLOCK_HZ / 64;
	uint32_t ovf = clock_hz / desired_sample_rate_hz; 

	// 16 bit timer overflow
	if (ovf > 0xFFFF)
	{
		fprintf(stderr, "warning: sample rate out of range, reverting to default\n");
		ovf = TIMER_OVF_DEFAULT;
	}

	*t_div = XMEGA_CLKDIV_64_gc;
	*t_ovf = (ovf-1); // -1 is extremely important! this is for overflow

	float filpot_r = 1.0 / ((desired_sample_rate_hz/2.0)*2.0*M_PI*FILCAP_F);
	*filpot_pos = (filpot_r / FILPOT_OHM) * 1024;
		
	return clock_hz / (*t_ovf+1);
} //}}}

float param_gain(uint32_t desired_gain, uint16_t* amppot_pos) //{{{
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
	return (100000.0 / ((((float)*amppot_pos)/1024.0) * AMPPOT_OHM)) + 1.0;
} //}}}
