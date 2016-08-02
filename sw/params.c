#include "common.h"

// set the BattOr's clock to the PC's RTC
int param_write_rtc()
{
	struct timeval tv;
	memset(&tv, 0, sizeof(tv));

	// wait until time is approx an even second
	do
	{
		gettimeofday(&tv, NULL);
	} while (tv.tv_usec > 1000);

	control(CONTROL_TYPE_SET_RTC,
		(tv.tv_sec & 0xFFFF0000) >> 16,
		(tv.tv_sec & 0x0000FFFF),
		1);
	return 0;
}

// compare the firmware and software git version
int param_check_version()
{
	uint8_t tries = 0;
	uint8_t type;
	uint8_t git_hash[GIT_HASH_LEN];

	while (tries++ < CONTROL_ATTEMPTS)
	{
		memset(git_hash, 0, sizeof(git_hash));
		control(CONTROL_TYPE_GET_GIT_HASH, 0, 0, 0);
		usleep(CONTROL_SLEEP_US); // since GIT HASH is sent in ACK, wait for it
		if (uart_rx_bytes(&type, git_hash, GIT_HASH_LEN) == GIT_HASH_LEN)
		{
			if (memcmp(git_hash, GIT_HASH, GIT_HASH_LEN) != 0)
			{
				fprintf(stderr, "fw version [%.7s] sw version [%.7s]\n",
					 git_hash, GIT_HASH);
				return -1;
			}
			break;
		}
	}

	if (tries >= CONTROL_ATTEMPTS)
		return -2;
	else
		return 0;
}

// read parameters from the battor's eeprom
int param_read_eeprom(eeprom_params* params)
{
	uint8_t tries = 0;
	uint8_t magic[4] = {0x31, 0x10, 0x31, 0x10};
	uint8_t type;

	while (tries++ < CONTROL_ATTEMPTS)
	{
		memset(params, 0, sizeof(eeprom_params));
		control(CONTROL_TYPE_READ_EEPROM, sizeof(eeprom_params), 0, 0);
		usleep(CONTROL_SLEEP_US); // since EEPROM is sent in ACK, wait for it
		uart_rx_bytes(&type, params, sizeof(eeprom_params));

		if (memcmp(magic, params->magic, sizeof(magic)) == 0)
		{
			verb_printf("param_read_eeprom:%s\n", "");
			verb_printf("\tmagic: %0X%0X%0X%0X\n", 
				params->magic[0],
				params->magic[1],
				params->magic[2],
				params->magic[3]);
			verb_printf("\tversion: %d\n", params->version);
			verb_printf("\tserialno: %s\n", params->serialno);
			verb_printf("\ttimestamp: %d\n", params->timestamp);
			verb_printf("\tR1: %f\n", params->R1);
			verb_printf("\tR2: %f\n", params->R2);
			verb_printf("\tR3: %f\n", params->R3);
			verb_printf("\tgainL: %f\n", params->gainL);
			verb_printf("\tgainL_R1corr: %f\n", params->gainL_R1corr);
			verb_printf("\tgainL_U1off: %f\n", params->gainL_U1off);
			verb_printf("\tgainL_amppot: %d\n", params->gainL_amppot);
			verb_printf("\tgainH: %f\n", params->gainH);
			verb_printf("\tgainH_R1corr: %f\n", params->gainH_R1corr);
			verb_printf("\tgainH_U1off: %f\n", params->gainH_U1off);
			verb_printf("\tgainH_amppot: %d\n", params->gainH_amppot);
			verb_printf("\tsd_sr: %d\n", params->sd_sr);
			verb_printf("\tsd_tdiv: %d\n", params->sd_tdiv);
			verb_printf("\tsd_tovf: %d\n", params->sd_tovf);
			verb_printf("\tsd_filpot: %d\n", params->sd_filpot);
			verb_printf("\tuart_sr: %d\n", params->uart_sr);
			verb_printf("\tuart_tdiv: %d\n", params->uart_tdiv);
			verb_printf("\tuart_tovf: %d\n", params->uart_tovf);
			verb_printf("\tuart_filpot: %d\n", params->uart_filpot);

			if (params->version >= EEPROM_PORT_AVG_2PWR_VER)
				verb_printf("\tport_avg_2pwr: %d\n", params->port_avg_2pwr);

			// TODO check the EEPROM CRC
			break;
		}
	}

	if (tries >= CONTROL_ATTEMPTS)
		return -1;
	else
		return 0;
}

char gain_to_char(enum PARAM_GAIN_enum gain)
{
	switch (gain)
	{
		case PARAM_GAIN_LOW: return 'L';
		case PARAM_GAIN_HIGH: return 'H';
		default: return 'E'; // Error
	}
}
