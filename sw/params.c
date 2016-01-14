#include "common.h"

// read parameters from the battor's eeprom
int param_read_eeprom(eeprom_params* params)
{
	uint8_t tries = 0;
	uint8_t magic[4] = {0x31, 0x10, 0x31, 0x10};
	uint8_t type;

	while (tries++ < CONTROL_EEPROM_ATTEMPTS)
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
			// TODO check the EEPROM CRC
			break;
		}
	}

	if (tries >= CONTROL_EEPROM_ATTEMPTS)
		return -1;
	else
		return 0;
}
