#include "common.h"

#include "uart.h"

void usage(char* name) //{{{
{
	fprintf(stderr, "\
BattOr's PC companion                                                               \n\n\
usage: %s -s <options>               stream power measurements over USB               \n\
   or: %s -d                         download power measurement from SD card          \n\
   or: %s -c <options>               format the SD card and upload the configuration  \n\
                                                                                      \n\
Options:                                                                              \n\
  -r <rate> : sample rate (default %d Hz)                                             \n\
  -g <gain> : current gain (default %dx) set to hit max then reduce                   \n\
  -o <offset> : current offset (default 0) set to adjust for the amplifier            \n\
                                                                                      \n\
", name, name, name, SAMPLE_RATE_HZ_DEFAULT, CURRENT_GAIN_DEFAULT);
} //}}}

int main(int argc, char** argv)
{
	FILE* file;
	char opt;
	char usb = 0, conf = 0, down = 0;

	uint16_t down_file;
	uint16_t timer_ovf, timer_div;
	uint16_t filpot_pos, amppot_pos;
	uint32_t	sample_rate = param_sample_rate(SAMPLE_RATE_HZ_DEFAULT, &timer_ovf, &timer_div, &filpot_pos);
	float gain = param_gain(CURRENT_GAIN_DEFAULT, &amppot_pos);
	float current_offset = 0;

	// need an option
	if (argc < 2)
	{
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	// process the options
	opterr = 0;
	while ((opt = getopt(argc, argv, "scd:r:g:o:hu:")) != -1)
	{
		switch(opt)
		{
			case 's':
				usb = 1;
			break;
			case 'r':
				if (atoi(optarg) != 0)
					sample_rate = param_sample_rate(atoi(optarg), &timer_ovf, &timer_div, &filpot_pos);
				else
				{
					usage(argv[0]);
					return EXIT_FAILURE;
				}
			break;
			case 'c':
				conf = 1;				
			break;
			case 'd':
				down = 1;
				down_file = atoi(optarg);
			break;
			case 'g':
				if (atoi(optarg) != 0)
					gain = param_gain(atoi(optarg), &amppot_pos);
				else
				{
					usage(argv[0]);
					return EXIT_FAILURE;
				}
			break;
			case 'o':
				if (atof(optarg) != 0.0)
				{
					current_offset = atof(optarg);
				}
				else
				{
					usage(argv[0]);
					return EXIT_FAILURE;
				}
			break;
			case 'u':
				if (atoi(optarg) == 0 || atoi(optarg) == 1)
				{
					uart_init();
					control(CONTROL_TYPE_USB_POWER_SET, atoi(optarg), 0);
					return EXIT_SUCCESS;
				}
				else
				{
					usage(argv[0]);
					return EXIT_FAILURE;
				}
			break;
			case 'h':
				usage(argv[0]);
				return EXIT_FAILURE;
			default:
				usage(argv[0]);
				return EXIT_FAILURE;
		}
	}

	sample min_s;
	sample max_s;
	min_s.signal = 0;
	max_s.signal = 2048;
	printf("# voltage range [%f, %f] mV\n", sample_v(&min_s), sample_v(&max_s));
	printf("# current range [%f, %f] mA\n", sample_i(&min_s, gain, current_offset), sample_i(&max_s, gain, current_offset));
	printf("# sample_rate=%dHz, gain=%fx\n", sample_rate, gain);
	printf("# filpot_pos=%d, amppot_pos=%d, timer_ovf=%d, timer_div=%d\n", filpot_pos, amppot_pos, timer_ovf, timer_div);

	uart_init();

	// download file
	if (down)
	{
		// read configuration
		// TODO
		// read file
		control(CONTROL_TYPE_READ_FILE, down_file, 0);
		samples_print_loop(gain, current_offset);
	}

	// start configuration recording if enabled
	if (conf)
	{
		srand(time(NULL));
		control(CONTROL_TYPE_START_REC_CONTROL, rand() & 0xFFFF, 0);
	}

	// configuration
	control(CONTROL_TYPE_FILPOT_SET, filpot_pos, 0);
	control(CONTROL_TYPE_AMPPOT_SET, amppot_pos, 0);
	control(CONTROL_TYPE_SAMPLE_TIMER_SET, timer_div, timer_ovf);

	// end configuration recording if enabled
	if (conf)
	{
		control(CONTROL_TYPE_START_SAMPLING_SD, 0, 0);
		control(CONTROL_TYPE_END_REC_CONTROL, 0, 0);
	}

	if (usb)
	{
		control(CONTROL_TYPE_START_SAMPLING_UART, 0, 0);
		samples_print_loop(gain, current_offset);
	}
	
	return EXIT_SUCCESS;
}
