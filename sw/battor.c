#include "common.h"

#include "uart.h"

char g_verb = 0;

void usage(char* name) //{{{
{
	fprintf(stderr, "\
BattOr's PC companion     \n\n\
usage: %s -s <options>    *stream* power measurements over USB              \n\
   or: %s -d              *download* power measurement from SD card         \n\
   or: %s -f <options>    *format* the SD card and upload the configuration \n\
   or: %s -k              *restart* the MCU                                 \n\
                                                                            \n\
Options:                                                                    \n\
  -r <rate> : sample rate (default %d Hz)                                   \n\
  -g <[L]ow or [H]igh> : current gain (default %c)                          \n\
  -v(v) : verbose printing for debugging                                    \n\
                                                                            \n\
Output:                                                                     \n\
  Each line is a power sample: <time (msec)> <current (mA)> <volatge (mV)>  \n\
  Min and max current (I) and voltage (V) are indicated by [m_] and [M_]    \n\
", name, name, name, name, SAMPLE_RATE_HZ_DEFAULT, GAIN_DEFAULT);
} //}}}

int main(int argc, char** argv)
{
	int i;
	FILE* file;
	char opt;
	char usb = 0, format = 0, down = 0, reset = 0, test = 0;

	uint16_t down_file;
	uint16_t timer_ovf, timer_div;
	uint16_t filpot_pos, amppot_pos;
	uint16_t ovs_bits = OVERSAMPLE_BITS_DEFAULT;
	double gain = 0.0;
	char gain_c = GAIN_DEFAULT;
	uint32_t sample_rate = SAMPLE_RATE_HZ_DEFAULT;
	eeprom_params eeparams;

	// need an option
	if (argc < 2)
	{
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	// process the options
	opterr = 0;
	while ((opt = getopt(argc, argv, "sfd:b:r:g:o:vhu:kt")) != -1)
	{
		switch(opt)
		{
			case 's':
				usb = 1;
			break;
			case 'r':
				if (atoi(optarg) != 0)
					sample_rate = atoi(optarg);
				else
				{
					usage(argv[0]);
					return EXIT_FAILURE;
				}
			break;
			case 'f':
				format = 1;				
			break;
			case 'd':
				down = 1;
				down_file = atoi(optarg);
			break;
			case 'g':
				gain_c = optarg[0]; 

				if(gain_c != 'L' && gain_c != 'H')
				{
					usage(argv[0]);
					return EXIT_FAILURE;
				}
			break;
			case 'b':
				ovs_bits = atoi(optarg);
				if (ovs_bits > OVERSAMPLE_BITS_MAX)
				{
					usage(argv[0]);
					return EXIT_FAILURE;
				}
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
			default:
				usage(argv[0]);
				return EXIT_FAILURE;
		}
	}

	samples_init(ovs_bits);

	uart_init();

	// init the battor 
	control(CONTROL_TYPE_INIT, 0, 0, 1);
	if (reset)
	{
		control(CONTROL_TYPE_RESET, 0, 0, 0);
		return EXIT_SUCCESS;
	}

	// read the BattOr's calibration params from its EEPROM
	memset(&eeparams, 0, sizeof(eeparams));
	if (param_read_eeprom(&eeparams) < 0)
	{
		fprintf(stderr, "Error: BattOr not calibrated or EEPROM failure\n");
		return EXIT_FAILURE;
	}

	// get actual sample rate
	sample_rate = param_sample_rate(sample_rate, ovs_bits, &timer_ovf, &timer_div, &filpot_pos);

	// set gain based on values in EEPROM, and set actual gain
	if (gain_c == 'L')
	{
		eeparams.gainL = param_gain((uint32_t)eeparams.gainL, &amppot_pos);
		gain = eeparams.gainL;
	}
	if (gain_c == 'H')
	{
		eeparams.gainH = param_gain((uint32_t)eeparams.gainH, &amppot_pos);
		gain = eeparams.gainH;
	}

	// print settings
	sample min_s;
	sample max_s;
	min_s.signal = 0;
	max_s.signal = (1 << (ADC_BITS + ovs_bits)) - 1;
	printf("# BattOr\n");
	printf("# voltage range [%f, %f] mV\n", sample_v(&min_s, &eeparams, 0.0, 0), sample_v(&max_s, &eeparams, 0.0, 0));
	printf("# current range [%f, %f] mA\n", sample_i(&min_s, &eeparams, 0.0, gain, 0), sample_i(&max_s, &eeparams, 0.0, gain, 0));
	printf("# sample_rate=%dHz, gain=%fx\n", sample_rate, gain);
	printf("# filpot_pos=%d, amppot_pos=%d, timer_ovf=%d, timer_div=%d ovs_bits=%d\n", filpot_pos, amppot_pos, timer_ovf, timer_div, ovs_bits);
	

	// download file
	if (down)
	{
		// read configuration
		control(CONTROL_TYPE_READ_FILE, down_file, 0, 1);
		samples_print_loop(&eeparams, gain, ovs_bits, g_verb, sample_rate, 0);
	}

	// start configuration recording if enabled
	if (format)
	{
		srand(time(NULL));
		control(CONTROL_TYPE_START_REC_CONTROL, rand() & 0xFFFF, 0, 1);
	}

	// configuration
	control(CONTROL_TYPE_FILPOT_SET, filpot_pos, 0, 1);
	control(CONTROL_TYPE_AMPPOT_SET, amppot_pos, 0, 1);
	control(CONTROL_TYPE_SAMPLE_TIMER_SET, timer_div, timer_ovf, 1);

	// end configuration recording if enabled
	if (format)
	{
		control(CONTROL_TYPE_START_SAMPLING_SD, 0, 0, 1);
		control(CONTROL_TYPE_END_REC_CONTROL, 0, 0, 1);
	}

	if (usb)
	{
		control(CONTROL_TYPE_START_SAMPLING_UART, 0, 0, 1);
		samples_print_loop(&eeparams, gain, ovs_bits, g_verb, sample_rate, test);
	}
	
	return EXIT_SUCCESS;
}
