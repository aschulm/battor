#include "common.h"

#include "uart.h"

char g_verb = 0;

void usage(char* name) //{{{
{
	fprintf(stderr, "\
BattOr's PC companion     \n\n\
usage: %s <tty> -s <options>    *stream* power measurements over USB              \n\
   or: %s <tty> -d              *download* power measurement from SD card         \n\
   or: %s <tty> -f <options>    *format* the SD card and upload the configuration \n\
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
", name, name, name, name, SAMPLE_RATE_HZ_DEFAULT, GAIN_DEFAULT);
} //}}}

int main(int argc, char** argv)
{
	int i;
	FILE* file;
	char opt;
	char* tty;
	char usb = 0, format = 0, down = 0, reset = 0, test = 0;

	uint16_t down_file;
	uint16_t timer_ovf, timer_div;
	uint16_t filpot_pos, amppot_pos;
	uint16_t ovs_bits = OVERSAMPLE_BITS_DEFAULT;
	char gain_c = GAIN_DEFAULT;
	samples_config sconf;
	eeprom_params* eeparams = &sconf.eeparams;

	samples_init(&sconf);

	// need an option
	if (argc < 3)
	{
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	// process the tty
	tty = argv[1];
	argv++;
	argc--;

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
					sconf.sample_rate = atoi(optarg);
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

	uart_init(tty);

    // run self test
    if (test)
    {
        int ret;
        if ((ret = control(CONTROL_TYPE_SELF_TEST, 0, 0, 1)) != 0) {
            fprintf(stderr, "Error: self test failed %d\n", ret);
            return EXIT_FAILURE;
        }
        fprintf(stderr, "Self test passed\n");
        return EXIT_SUCCESS;
    }


	// init the battor 
	control(CONTROL_TYPE_INIT, 0, 0, 1);

	if (reset)
	{
		control(CONTROL_TYPE_RESET, 0, 0, 0);
		return EXIT_SUCCESS;
	}

	// read the BattOr's calibration params from its EEPROM
	if (param_read_eeprom(eeparams) < 0)
	{
		fprintf(stderr, "Error: EEPROM read failure or BattOr not calibrated\n");
		return EXIT_FAILURE;
	}

	// get actual sample rate
	sconf.sample_rate = param_sample_rate(sconf.sample_rate, ovs_bits, &timer_ovf, &timer_div, &filpot_pos);

	// set gain based on values in EEPROM, and set actual gain
	if (gain_c == 'L')
	{
		eeparams->gainL = param_gain((uint32_t)eeparams->gainL, &amppot_pos);
		sconf.gain = eeparams->gainL;
	}
	if (gain_c == 'H')
	{
		eeparams->gainH = param_gain((uint32_t)eeparams->gainH, &amppot_pos);
		sconf.gain = eeparams->gainH;
	}

	// print settings
	sample min_s;
	sample max_s;
	min_s.signal = 0;
	max_s.signal = (1 << (ADC_BITS + ovs_bits)) - 1;
	printf("# BattOr\n");
	printf("# voltage range [%f, %f] mV\n", sample_v(&min_s, &sconf, 0.0, 0), sample_v(&max_s, &sconf, 0.0, 0));
	printf("# current range [%f, %f] mA\n", sample_i(&min_s, &sconf, 0.0, 0), sample_i(&max_s, &sconf, 0.0, 0));
	printf("# sample_rate=%dHz, gain=%fx\n", sconf.sample_rate, sconf.gain);
	printf("# filpot_pos=%d, amppot_pos=%d, timer_ovf=%d, timer_div=%d ovs_bits=%d\n", filpot_pos, amppot_pos, timer_ovf, timer_div, ovs_bits);
	

	// download file
	if (down)
	{
		// read configuration
		control(CONTROL_TYPE_READ_FILE, down_file, 0, 1);
		samples_print_loop(&sconf);
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
		samples_print_loop(&sconf);
	}
	
	return EXIT_SUCCESS;
}
