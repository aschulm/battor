#include "common.h"

#include "uart.h"

char g_verb = 0;

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
  -b <bits> : set the number of bits to obtain through oversampling (default %d max 1)\n\
  -v : verbose printing for debugging                                                 \n\
                                                                                      \n\
", name, name, name, SAMPLE_RATE_HZ_DEFAULT, CURRENT_GAIN_DEFAULT, OVERSAMPLE_BITS_DEFAULT);
} //}}}

int main(int argc, char** argv)
{
	int i;
	FILE* file;
	char opt;
	char usb = 0, conf = 0, down = 0;

	uint16_t down_file;
	uint16_t timer_ovf, timer_div;
	uint16_t filpot_pos, amppot_pos;
	uint16_t ovs_bits = OVERSAMPLE_BITS_DEFAULT;
	double gain = param_gain(CURRENT_GAIN_DEFAULT, &amppot_pos);
	double current_offset = 0;
	uint32_t	sample_rate = SAMPLE_RATE_HZ_DEFAULT;

	// need an option
	if (argc < 2)
	{
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	// process the options
	opterr = 0;
	while ((opt = getopt(argc, argv, "scd:b:r:g:o:vhu:")) != -1)
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
			case 'c':
				conf = 1;				
			break;
			case 'd':
				down = 1;
				down_file = atoi(optarg);
			break;
			case 'g':
				if (atoi(optarg) != 0)
					gain = atoi(optarg); 
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
			case 'b':
				ovs_bits = atoi(optarg);
				if (ovs_bits > OVERSAMPLE_BITS_MAX)
				{
					usage(argv[0]);
					return EXIT_FAILURE;
				}
			break;
			case 'v':
				g_verb = 1;
			break;
			case 'h':
				usage(argv[0]);
				return EXIT_FAILURE;
			default:
				usage(argv[0]);
				return EXIT_FAILURE;
		}
	}

	// get actual parameters
	sample_rate = param_sample_rate(sample_rate, ovs_bits, &timer_ovf, &timer_div, &filpot_pos);
	gain = param_gain(gain, &amppot_pos);

	samples_init(ovs_bits);

	sample min_s;
	sample max_s;
	min_s.signal = 0;
	max_s.signal = 1 << (ADC_BITS + ovs_bits);
	printf("# voltage range [%f, %f] mV\n", sample_v(&min_s), sample_v(&max_s));
	printf("# current range [%f, %f] mA\n", sample_i(&min_s, gain, current_offset), sample_i(&max_s, gain, current_offset));
	printf("# sample_rate=%dHz, gain=%fx\n", sample_rate, gain);
	printf("# filpot_pos=%d, amppot_pos=%d, timer_ovf=%d, timer_div=%d ovs_bits=%d\n", filpot_pos, amppot_pos, timer_ovf, timer_div, ovs_bits);

	uart_init();

	// init the battor 
	control(CONTROL_TYPE_INIT, 0, 0);

	// download file
	if (down)
	{
		// read configuration
		control(CONTROL_TYPE_READ_FILE, down_file, 0);
		samples_print_loop(gain, current_offset, ovs_bits, g_verb, sample_rate);
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
	control(CONTROL_TYPE_OVERSAMPLING_SET, ovs_bits, 0);

	// end configuration recording if enabled
	if (conf)
	{
		control(CONTROL_TYPE_START_SAMPLING_SD, 0, 0);
		control(CONTROL_TYPE_END_REC_CONTROL, 0, 0);
	}

	if (usb)
	{
		control(CONTROL_TYPE_START_SAMPLING_UART, 0, 0);
		samples_print_loop(gain, current_offset, ovs_bits, g_verb, sample_rate);
	}
	
	return EXIT_SUCCESS;
}
