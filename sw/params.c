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
		fprintf(stderr, "error: gain out of range\n");
		exit(EXIT_FAILURE);
	}

	*amppot_pos = (uint16_t)(((100000.0/(desired_gain - 1.0)) / AMPPOT_OHM) * 1024.0);

	if (*amppot_pos == 0 || *amppot_pos > 1023)
	{
		fprintf(stderr, "error: gain out of range\n");
		exit(EXIT_FAILURE);
	}
	return (100000.0 / ((((double)*amppot_pos)/1024.0) * AMPPOT_OHM)) + 1.0;
} //}}}

// read parameters from the battor's eeprom
int param_read_eeprom(eeprom_params* params)
{
	uint8_t magic[4] = {0x31, 0x10, 0x31, 0x10};
	uint8_t type;
	control(CONTROL_TYPE_READ_EEPROM, sizeof(eeprom_params), 0, 0);
	usleep(CONTROL_SLEEP_US); // since EEPROM is sent in ACK, wait for it
	uart_rx_bytes(&type, params, sizeof(eeprom_params));

	if (memcmp(magic, params->magic, sizeof(magic)) != 0)
		return -1;

	verb_printf("param_read_eeprom:%s\n", "");
	verb_printf("\tmagic: %0X%0X%0X%0X\n", 
		params->magic[0],
		params->magic[1],
		params->magic[2],
		params->magic[3]);
	verb_printf("\tversion: %d\n", params->version);
	verb_printf("\ttimestamp: %d\n", params->timestamp);
	verb_printf("\tR1: %f\n", params->R1);
	verb_printf("\tR2: %f\n", params->R2);
	verb_printf("\tR3: %f\n", params->R3);
	verb_printf("\tgainL: %f\n", params->gainL);
	verb_printf("\tgainL_R1corr: %f\n", params->gainL_R1corr);
	verb_printf("\tgainL_U1off: %f\n", params->gainL_U1off);
	verb_printf("\tgainH: %f\n", params->gainH);
	verb_printf("\tgainH_R1corr: %f\n", params->gainH_R1corr);
	verb_printf("\tgainH_U1off: %f\n", params->gainH_U1off);

	// TODO check the EEPROM CRC
	return 0;
}
