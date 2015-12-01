#include "common.h"

#include "uart.h"

char g_verb = 0;

void usage(char* name) //{{{
{
	fprintf(stderr, "\
BattOr's PC companion     \n\n\
usage: %s <tty> -s <options>    *stream* power measurements over USB              \n\
   or: %s <tty> -b <options>    *buffer* on the SD card and download on CTRL+C    \n\
   or: %s <tty> -k              *restart* the MCU                                 \n\
                                                                            \n\
Options:                                                                    \n\
  -r <rate> : sample rate (default %d Hz)                                   \n\
  -g <[L]ow or [H]igh> : current gain (default %c)                          \n\
  -v(v) : verbose printing for debugging                                    \n\
                                                                            \n\
Output:                                                                     \n\
  Each line is a power sample: <time (msec)> <current (mA)> <volatge (mV)>  \n\
  Min and max current (I) and voltage (V) are indicated by [m_] and [M_]    \n\
", name, name, name, SAMPLE_RATE_HZ_DEFAULT, GAIN_DEFAULT);
} //}}}

int main(int argc, char** argv)
{
	int i;
	FILE* file;
	char opt;
	char tty[] = "/dev/ttyUSB0";
	char usb = 0, buffer = 0, reset = 0, test = 0, cal = 0;

	uint16_t down_file;
	uint16_t timer_ovf, timer_div;
	uint16_t filpot_pos, amppot_pos;
	uint16_t ovs_bits = OVERSAMPLE_BITS_DEFAULT;
	char gain_c;
	uint16_t gain = GAIN_DEFAULT;
	samples_config sconf;
	eeprom_params* eeparams = &sconf.eeparams;

	samples_init(&sconf);

	// need an option
	if (argc < 2)
	{
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	// process the options
	opterr = 0;
	while ((opt = getopt(argc, argv, "sbr:g:o:vhu:ktc")) != -1)
	{
		switch(opt)
		{
			case 'c':
				sconf.cal = 1;
			break;
			case 's':
				usb = 1;
			break;
			case 'r':
				if (atoi(optarg) != 0)
					sconf.sample_rate = atoi(optarg);
				else
				{
					usage(argv[0]);
					return EXIT_FAILURE;
				}
			break;
			case 'g':
				gain_c = optarg[0]; 

				if (gain_c == 'L')
					gain = PARAM_GAIN_LOW;
				else if (gain_c == 'H')
					gain = PARAM_GAIN_HIGH;
				else
				{
					usage(argv[0]);
					return EXIT_FAILURE;
				}
			break;
			case 'b':
				buffer = 1;
			break;
			case 'k':
				reset = 1;
			break; 
			case 't':
				test = 1;
			break;
			case 'v':
				g_verb++;
			break;
			case 'h':
				usage(argv[0]);
				return EXIT_FAILURE;
      break;
			default:
				usage(argv[0]);
				return EXIT_FAILURE;
		}
	}

	uart_init(tty);

	// run self test
	if (test)
	{
		int ret;
		if ((ret = control(CONTROL_TYPE_SELF_TEST, 0, 0, 1)) != 0) {
			fprintf(stderr, "++++++ Self Test FAILED %d ++++++\n", ret);
			return EXIT_FAILURE;
		}
		fprintf(stderr, "------ Self Test PASSED ------\n");
		return EXIT_SUCCESS;
	}

	if (reset)
	{
		control(CONTROL_TYPE_RESET, 0, 0, 0);
		return EXIT_SUCCESS;
	}

	// init the battor 
	control(CONTROL_TYPE_INIT, 0, 0, 1);

	// read the BattOr's calibration params from its EEPROM
	if (param_read_eeprom(eeparams) < 0)
	{
		fprintf(stderr, "Error: EEPROM read failure or BattOr not calibrated\n");
		return EXIT_FAILURE;
	}

	// get actual sample rate
	sconf.sample_rate = param_sample_rate(sconf.sample_rate, ovs_bits, &timer_ovf, &timer_div, &filpot_pos);


	// get gain setting from EEPROM
	if (gain == PARAM_GAIN_LOW)
		sconf.gain = eeparams->gainL;
	if (gain == PARAM_GAIN_HIGH)
		sconf.gain = eeparams->gainH;

	// print settings
	sample min_s;
	sample max_s;

	memset(&min_s, 0, sizeof(sample));
	max_s.v = (1 << (ADC_BITS + ovs_bits)) - 1;
	max_s.i = (1 << (ADC_BITS + ovs_bits)) - 1;
	printf("# BattOr\n");
	printf("# voltage range [%f, %f] mV\n", sample_v(&min_s, &sconf, 0.0), sample_v(&max_s, &sconf, 0.0));
	printf("# current range [%f, %f] mA\n", sample_i(&min_s, &sconf, 0.0), sample_i(&max_s, &sconf, 0.0));
	printf("# sample_rate=%dHz, gain=%fx\n", sconf.sample_rate, sconf.gain);
	printf("# filpot_pos=%d, amppot_pos=%d, timer_ovf=%d, timer_div=%d ovs_bits=%d\n", filpot_pos, amppot_pos, timer_ovf, timer_div, ovs_bits);
	
	// configuration
	control(CONTROL_TYPE_GAIN_SET, gain, 0, 1);
	control(CONTROL_TYPE_FILPOT_SET, filpot_pos, 0, 1);
	control(CONTROL_TYPE_SAMPLE_TIMER_SET, timer_div, timer_ovf, 1);

	if (usb)
	{
		control(CONTROL_TYPE_START_SAMPLING_UART, 0, 0, 1);
		samples_print_loop(&sconf);
	}

	if (buffer)
	{
		control(CONTROL_TYPE_START_SAMPLING_SD, 0, 0, 1);
		samples_print_loop(&sconf);
	}
	
	return EXIT_SUCCESS;
}
