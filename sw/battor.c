#include "common.h"

#include "uart.h"

char g_verb = 0;

void usage(char* name) //{{{
{
	fprintf(stderr, "\
BattOr's PC companion     \n\n\
usage: %s -s <options>     <tty> *stream* power measurements over USB                   \n\
   or: %s -b <options>     <tty> *buffer* on the SD card                                \n\
   or: %s -d <file number> <tty> *download* last file or <file number> from the SD card \n\
   or: %s -k               <tty> *restart* the MCU                                      \n\
   or: %s -o               <tty> *count* reuurn the sample count                        \n\
                                                                                          \n\
Options:                                                                                  \n\
  -g <[L]ow or [H]igh> : current gain (default %c)                                        \n\
  -c calibration mode                                                                     \n\
  -v(v) : verbose printing for debugging                                                  \n\
                                                                                          \n\
Output:                                                                                   \n\
  Each line is a power sample: <time (msec)> <current (mA)> <volatge (mV)>                \n\
  Min and max current (I) and voltage (V) are indicated by [m_] and [M_]                  \n\
", name, name, name, name, name, gain_to_char(GAIN_DEFAULT));
} //}}}

int main(int argc, char** argv)
{
	int i;
	FILE* file;
	char opt;
	char* tty = DEFAULT_TTY;
	char usb = 0, buffer = 0, reset = 0, test = 0, cal = 0, down = 0, count = 0;

	uint16_t down_file = 0;
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
	while ((opt = getopt(argc, argv, ":sbg:ovhktcd:")) != -1)
	{
		switch(opt)
		{
			case 'd':
				down = 1;
				down_file = strtol(optarg, NULL, 10);

				// tty passed as last parameter, not file number
				if(optind == argc && optarg[0] == '/')
					optind--;
			break;
			case 'o':
				count = 1;
			break;
			case 'c':
				sconf.cal = 1;
			break;
			case 's':
				usb = 1;
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

			// required argument not supplied
			case ':':
				switch (optopt)
				{
					case 'd':
						down = 1;
					break;
					default:
						usage(argv[0]);
						return EXIT_FAILURE;
				}
			break;

			default:
				usage(argv[0]);
				return EXIT_FAILURE;
		}
	}

	if (optind == argc - 1)
	{
		tty = argv[optind];
	}
	fprintf(stderr, "Connecting to BattOr on port %s\n", tty);

	uart_init(tty);

	// needs to be before firmware check to update firmware
	if (reset)
	{
		control(CONTROL_TYPE_RESET, 0, 0, 0);
		return EXIT_SUCCESS;
	}

	// check the firmware version
	if (param_check_version() < 0)
	{
		fprintf(stderr, "Error: Firmware software version mismatch, please reflash firmware.\n");
		return EXIT_FAILURE;
	}

	// run self test
	if (test)
	{
		int ret;
		if ((ret = control(CONTROL_TYPE_SELF_TEST, 0, 0, 1)) != 0)
		{
			fprintf(stderr, "++++++ Self Test FAILED %d ++++++\n", ret);
			return EXIT_FAILURE;
		}

		fprintf(stderr, "------ Self Test PASSED ------\n");
		return EXIT_SUCCESS;
	}

	// return the count
	if (count)
	{
		uint32_t sample_count;
		uint8_t type;

		control(CONTROL_TYPE_GET_SAMPLE_COUNT, 0, 0, 0);
		uart_rx_bytes(&type, &sample_count, sizeof(sample_count));
		printf("%u\n", sample_count);
		return EXIT_SUCCESS;
	}

	if (down)
	{
		// read the BattOr's calibration params from its EEPROM
		if (param_read_eeprom(eeparams) < 0)
		{
			fprintf(stderr, "Error: EEPROM read failure or BattOr not calibrated\n");
			return EXIT_FAILURE;
		}

		uint8_t avg_shift = 0;
		if (control(CONTROL_TYPE_GET_MODE_PORTABLE, 0, 0, 1))
			avg_shift = eeparams->port_avg_2pwr;
		else if (down_file > 0)
		{
			fprintf(stderr, "Error: In USB buffering mode, can only download last file.\n");
			return EXIT_FAILURE;
		}

		sconf.sample_rate = (uint32_t)(eeparams->sd_sr >> avg_shift);
		// TODO set proper gain!
		sconf.gain = eeparams->gainL;
		control(CONTROL_TYPE_READ_SD_UART, down_file, 0, 0);
		samples_print_loop(&sconf);
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
	
	// configuration
	control(CONTROL_TYPE_GAIN_SET, gain, 0, 1);

	if (usb)
	{
		sconf.sample_rate = eeparams->uart_sr;
		control(CONTROL_TYPE_START_SAMPLING_UART, 0, 0, 1);
		samples_print_loop(&sconf);
	}

	if (buffer)
	{
		control(CONTROL_TYPE_START_SAMPLING_SD, 0, 0, 1);
	}

	return EXIT_SUCCESS;
}
