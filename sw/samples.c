#include "common.h"
#include "uart.h"
#include "params.h"
#include "samples.h"

static double s_adc_top_pos;
static double s_adc_top_neg;

static inline double adc_to_v(int16_t s) //{{{
{
	double frac = (s > 0) ? (s / s_adc_top_pos) : (s / s_adc_top_neg);
	return frac * VREF;
} //}}}

void samples_init(samples_config* conf) //{{{
{
	// determine ADC_TOP with ovsersampling
	s_adc_top_pos = pow(2, (ADC_BITS + conf->ovs_bits))-1;
	s_adc_top_neg = pow(2, (ADC_BITS + conf->ovs_bits));
} //}}}

static inline double sample_v(sample* s, samples_config* conf, double cal_v) //{{{
{
	double v_v = adc_to_v(s->v) - cal_v;

	// warnings for minimum and maximum voltage
	if (v_v >= VREF)
		verb_printf("%s\n", "[Mv]");
	if (v_v < cal_v)
		verb_printf("%s\n", "[mv]");

	return (v_v / V_DEV(conf->eeparams.R2, conf->eeparams.R3)) * 1000.0; // undo the voltage divider
} //}}}

static inline double sample_i(sample* s, samples_config* conf, double cal_v) //{{{
{
	double i_v = adc_to_v(s->i) - cal_v;

	// warnings for minimum and maximum current
	if (i_v >= VREF)
		verb_printf("%s\n", "[Mi]");
	if (i_v < cal_v)
		verb_printf("%s\n", "[mi]");

	double i_adcv_unamp = i_v / conf->gain; // undo the current gain
	double i_samp = ((i_adcv_unamp / conf->eeparams.R1) * 1000.0);

	// the ordering of these is important
	if (conf->gain == conf->eeparams.gainL)
	{
		i_samp -= conf->eeparams.gainL_U1off;
		i_samp = i_samp / conf->eeparams.gainL_R1corr;
	}
	if (conf->gain == conf->eeparams.gainH)
	{
		i_samp -= conf->eeparams.gainH_U1off;
		i_samp = i_samp / conf->eeparams.gainH_R1corr;
	}
	return i_samp;
} //}}}

int16_t samples_read(sample* samples, samples_config* conf, uint32_t* seqnum) //{{{
{
	samples_hdr* hdr;
	uint8_t frame[50000];
	uint8_t* frame_ptr;
	uint8_t type;
	int ret = 0;


	if ((ret = uart_rx_bytes(&type, frame, sizeof(frame))) < 0 || type != UART_TYPE_SAMPLES)
		return 0;

	frame_ptr = frame;

	hdr = (samples_hdr*)frame_ptr;
	frame_ptr += sizeof(samples_hdr);

	verb_printf("samples_read: read_len:%d hdr_len:%d seqnum:%d\n", ret, hdr->len, hdr->seqnum);

	if (*seqnum != 0)
	{
		if (hdr->seqnum != *seqnum + 1)
			fprintf(stderr, "warning: expected sample frame seqnum %d got seqnum %d\n", *seqnum + 1, hdr->seqnum);
	}

	// end of file read
	if (hdr->len == 0)
		return -1;

	*seqnum = hdr->seqnum;

	memcpy(samples, frame_ptr, hdr->len);
	frame_ptr += hdr->len;

	return hdr->len / sizeof(sample);
} //}}}

void samples_print_config(samples_config* conf) //{{{
{
	sample min_s;
	sample max_s;
	memset(&min_s, 0, sizeof(sample));
	max_s.v = (1 << (ADC_BITS + conf->ovs_bits)) - 1;
	max_s.i = (1 << (ADC_BITS + conf->ovs_bits)) - 1;
	printf("# BattOr\n");
	printf("# voltage range [%f, %f] mV\n", sample_v(&min_s, conf, 0.0), sample_v(&max_s, conf, 0.0));
	printf("# current range [%f, %f] mA\n", sample_i(&min_s, conf, 0.0), sample_i(&max_s, conf, 0.0));
} //}}}

void samples_print_loop(samples_config* conf) //{{{
{
	int i;
	sample samples[2000];
	uint32_t seqnum = 0xFFFFFFFF;
	uint32_t sample_num = 0;
	int16_t samples_len = 0;
	double v_cal = 0, i_cal = 0;
	sigset_t sigs;
	double test_val = 0.0;

	// will block and unblock SIGINT
	sigemptyset(&sigs);
	sigaddset(&sigs, SIGINT);

	// read calibration and compute it
	while (samples_len == 0 || seqnum != 0)
	{
		samples_len = samples_read(samples, conf, &seqnum);

		if (samples_len == -1)
		{
			fprintf(stderr, "ERROR: the file can not be found in the BattOr's storage\n");
			exit(EXIT_FAILURE);
		}
	}

	if (samples_len == 0)
	{
		fprintf(stderr, "ERROR: read zero calibration samples\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < samples_len; i++)
	{
		verb_printf("adc_v_cal %d\n", samples[i].v);
		verb_printf("adc_i_cal %d\n", samples[i].i);

		v_cal += adc_to_v(samples[i].v);
		i_cal += adc_to_v(samples[i].i);
	}
	v_cal = v_cal / samples_len;
	i_cal = i_cal / samples_len;
	verb_printf("adc_v_cal_avg_v %f adc_i_cal_avg_v %f\n", v_cal, i_cal);

	// main sample read loop
	while(1)
	{
		while ((samples_len = samples_read(samples, conf, &seqnum)) == 0);

		// end of file, quit read loop
		if (samples_len <= 0)
			return;

		sigprocmask(SIG_BLOCK, &sigs, NULL);   // disable interrupts before print

		for (i = 0; i < samples_len; i++)
		{
			verb_printf("adc_v %d\n", samples[i].v);
			verb_printf("adc_i %d\n", samples[i].i);

			double msec =
				(sample_num / ((double)conf->sample_rate/pow(4, conf->ovs_bits))) * 1000.0;

			double mv = sample_v(samples + i, conf, v_cal);
			double mi = sample_i(samples + i, conf, i_cal);

			printf("%0.2f %0.1f %0.1f", msec, mi, mv);

			// check for test value on STDIN and print
			if (conf->cal)
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

		// flush the samples and enable interrupts again
		fflush(stdout);
		sigprocmask(SIG_UNBLOCK, &sigs, NULL);
	}
}
