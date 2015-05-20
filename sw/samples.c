#include "common.h"
#include "uart.h"
#include "params.h"
#include "samples.h"

static double s_adc_top;

void samples_init(uint16_t ovs_bits) //{{{
{
	// determine ADC_TOP with ovsersampling
	s_adc_top = pow(2, (ADC_BITS + ovs_bits));
	verb_printf("adc_top %f\n", s_adc_top);
} //}}}

double sample_v(sample* s, eeprom_params* eeparams, double cal, uint8_t warning) //{{{
{
	if (warning && s->signal == s_adc_top)
		fprintf(stderr, "WARNING: maximum voltage, won't hurt anything, but what phone battery has such a high voltage?\n");

	double v_adcv = (((double)s->signal - cal) / s_adc_top) * VREF;
	return (v_adcv / V_DEV(eeparams->R2, eeparams->R3)) * 1000.0; // undo the voltage divider
} //}}}

double sample_i(sample* s, eeprom_params* eeparams, double cal, double gain, uint8_t warning) //{{{
{
	// current
	if (warning && s->signal == s_adc_top)
		fprintf(stderr, "WARNING: maximum current, won't hurt anything, but you should turn down the gain.\n");

	double i_adcv = (((double)s->signal - cal) / s_adc_top) * VREF;
	double i_adcv_unamp = i_adcv / gain; // undo the current gain
	double i_samp = ((i_adcv_unamp / eeparams->R1) * 1000.0);

	// the ordering of these is important
	i_samp -= eeparams->gainL_U1off;
	i_samp = i_samp / eeparams->gainL_R1corr;
	return i_samp;
} //}}}

int16_t samples_read(sample* v_s, sample* i_s, uint32_t* seqnum) //{{{
{
	samples_hdr* hdr;
	uint8_t frame[50000];
	uint8_t* frame_ptr;
	uint8_t type;

	control(CONTROL_TYPE_READ_READY, 0, 0, 0);

	if (uart_rx_bytes(&type, frame, sizeof(frame)) < 0 || type != UART_TYPE_SAMPLES)
		return 0;

	frame_ptr = frame;

	hdr = (samples_hdr*)frame_ptr;
	frame_ptr += sizeof(samples_hdr);

	verb_printf("samples_read: len:%d seqnum:%d\n", hdr->samples_len, hdr->seqnum);

	if (*seqnum != 0)
	{
		if (hdr->seqnum != *seqnum + 1)
			fprintf(stderr, "warning: expected sample frame seqnum %d got seqnum %d\n", *seqnum + 1, hdr->seqnum);
	}

	// end of file read
	if (hdr->samples_len == 0)
		return -1;

	*seqnum = hdr->seqnum;

	memcpy(v_s, frame_ptr, hdr->samples_len);
	frame_ptr += hdr->samples_len;

	memcpy(i_s, frame_ptr, hdr->samples_len);
	frame_ptr += hdr->samples_len;

	return hdr->samples_len / sizeof(sample);
} //}}}

void samples_print_loop(eeprom_params* eeparams, double gain, double ovs_bits, char verb, uint32_t sample_rate, char test) //{{{
{
	int i;
	sample v_s[2000], i_s[2000];
	uint32_t seqnum = 0;
	uint32_t sample_num = 0;
	int16_t samples_len = 0;
	double v_cal = 0, i_cal = 0;
	sigset_t sigs;
	double test_val = 0.0;

	// will block and unblock SIGINT
	sigemptyset(&sigs);
	sigaddset(&sigs, SIGINT);

	// read calibration and compute it
	while (samples_len == 0 || seqnum != 1)
	{
		samples_len = samples_read(v_s, i_s, &seqnum);
	}

	if (samples_len == 0)
	{
		fprintf(stderr, "ERROR: read zero samples\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < samples_len; i++)
	{
		verb_printf("v_cal %d\n", v_s[i].signal);
		verb_printf("i_cal %d\n", i_s[i].signal);

		v_cal += v_s[i].signal;
		i_cal += i_s[i].signal;
	}
	v_cal = v_cal / samples_len;
	i_cal = i_cal / samples_len;
	verb_printf("v_cal_avg %f, i_cal_avg %f\n", v_cal, i_cal);	

	// main sample read loop
	while(1)
	{
		while ((samples_len = samples_read(v_s, i_s, &seqnum)) == 0);

		// end of file, quit read loop
		if (samples_len < 0)
			return;

		sigprocmask(SIG_BLOCK, &sigs, NULL);   // disable interrupts before print

		for (i = 0; i < samples_len; i++)
		{
			verb_printf("i %d %f\n", i_s[i].signal, i_s[i].signal - i_cal);
			verb_printf("v %d %f\n", v_s[i].signal, v_s[i].signal - v_cal);

			double msec = (sample_num/((double)sample_rate)) * 1000.0;
			double mv = sample_v(v_s + i, eeparams, v_cal, 1);
			double mi = sample_i(i_s + i, eeparams, i_cal, gain, 1);

			printf("%f %f %f", msec, mi, mv);

			// check for test value on STDIN and print
			if (test)
			{
				struct timeval tv;
				fd_set fds;
				int ret;

				tv.tv_sec = 0;
				tv.tv_usec = 0;
				FD_ZERO(&fds);
				FD_SET(STDIN_FILENO, &fds);
				select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);

				if (FD_ISSET(STDIN_FILENO, &fds))
				{
					ret = fscanf(stdin, "%lf", &test_val);
				}
				printf(" %f", test_val);
			}

			printf("\n");

			sample_num++;
		}

		fflush(stdout);
		sigprocmask(SIG_UNBLOCK, &sigs, NULL); // enable interrupts before print
	}
}
