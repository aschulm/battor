#include "common.h"

#include "control.h"
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
	else if (gain == PARAM_GAIN_CAL)
		amppot_set = POT_WIPERPOS_MAX;
	else
		halt(ERROR_AMPPOT_SET_FAILED);

	// Set the potentiometer setting
	pot_wiperpos_set(POT_AMP_CS_PIN_gm, amppot_set);
	amppot_get = pot_wiperpos_get(POT_AMP_CS_PIN_gm);
	if (amppot_get != amppot_set)
		halt(ERROR_AMPPOT_SET_FAILED);
	
	return 0;
}

void params_set_samplerate()
{
	uint16_t tovf = 0, tdiv = 0;
	uint16_t filpot_set = 0, filpot_get = 0;

	if (g_control_mode == CONTROL_MODE_STREAM)
	{
		tovf = eeprom.uart_tovf;
		tdiv = eeprom.uart_tdiv;
		filpot_set = eeprom.uart_filpot;
	}
	else if (g_control_mode == CONTROL_MODE_USB_STORE ||
		g_control_mode == CONTROL_MODE_PORT_STORE)
	{
		tovf = eeprom.sd_tovf;
		tdiv = eeprom.sd_tdiv;
		filpot_set = eeprom.sd_filpot;
	}
	else
		halt(ERROR_SAMPLERATE_SET_WRONG_MODE);

	// set timer prescaler and period
	timer_set(&TCD0, tdiv, tovf);

	// set filter potentiomter
	pot_wiperpos_set(POT_FIL_CS_PIN_gm, filpot_set);
	filpot_get = pot_wiperpos_get(POT_FIL_CS_PIN_gm);
	if (filpot_get != filpot_set)
		halt(ERROR_FILPOT_SET_FAILED);
}

int8_t params_get_port_avg_2pwr()
{
	if (eeprom.version < PARAMS_EEPROM_PORT_AVG_2PWR_VER)
		return -1;

	return eeprom.port_avg_2pwr;
}
