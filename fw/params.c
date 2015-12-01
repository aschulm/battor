#include "common.h"

#include "error.h"
#include "params.h"

static eeprom_params eeprom;

void params_init()
{
	// Read the parameters in the EEPROM into local memory
	EEPROM_read_block(0, (uint8_t*)&eeprom, sizeof(eeprom));	
}

int params_set_gain(uint8_t gain)
{
	uint16_t amppot_set = 0, amppot_get = 0;

	// Get the potentiometer setting for the gain from the EEPROM
	if (gain == PARAM_GAIN_LOW)
		amppot_set = eeprom.gainL_amppot;
	else if (gain == PARAM_GAIN_HIGH)
		amppot_set = eeprom.gainH_amppot;
	else
		return -1;

	// Set the potentiometer setting
	pot_wiperpos_set(POT_AMP_CS_PIN_gm, amppot_set);
	amppot_get = pot_wiperpos_get(POT_AMP_CS_PIN_gm);
	if (amppot_get != amppot_set)
		halt(ERROR_AMPPOT_SET_FAILED);
	
	return 0;
}
