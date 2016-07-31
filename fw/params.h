#ifndef PARAMS_H
#define PARAMS_H

#define PARAMS_EEPROM_PORT_AVG_2PWR_VER 1

/* 
 * Note: all floats in the EEPROM are treated as integers
 * so they can be skipped over by the firmware.
 * 
 * Do not read the variables marked as float in the firmware!
 */
struct eeprom_params_
{
	uint8_t magic[4];         // Magic (0x31103110)
	uint16_t version;         // Version (1)
	char serialno[20];        // Serial number
	uint32_t timestamp;       // Calibration timestamp (epoch time in secs)
	uint32_t R1;              // [float] R1 [current sense] value
	uint32_t R2;              // [float] R2 [V divider numerator] value
	uint32_t R3;              // [float] R3 [V divider denominator] value
	uint32_t gainL;           // [float] Low gain value
	uint32_t gainL_R1corr;    // [float] Low gain R1 correction
	uint32_t gainL_U1off;     // [float] Low gain U1 offset correction
	uint16_t gainL_amppot;    // Low gain amplifier potentiometer position
	uint32_t gainH;           // [float] High gain value
	uint32_t gainH_R1corr;    // [float] High gain R1 correction
	uint32_t gainH_U1off;     // [float] High gain U1 offset correction
	uint16_t gainH_amppot;    // High gain amplifier potentiometer position
	uint32_t sd_sr;           // SD sample rate
	uint16_t sd_tdiv;         // SD timer divider
	uint16_t sd_tovf;         // SD timer overflow
	uint16_t sd_filpot;       // SD filter potentiometer position
	uint32_t uart_sr;         // UART sample rate
	uint16_t uart_tdiv;       // UART timer divider
	uint16_t uart_tovf;       // UART timer overflow
	uint16_t uart_filpot;     // UART filter potentiometer position
	uint8_t port_avg_2pwr;    // power of 2 to average portable samples
	uint32_t crc32;           // CRC32 [zip algorithm]
} __attribute__((packed));
typedef struct eeprom_params_ eeprom_params;

typedef enum PARAM_GAIN_enum
{
	PARAM_GAIN_LOW = 0,
	PARAM_GAIN_HIGH,
	PARAM_GAIN_CAL = 255,
} PARAM_GAIN_t;

void params_init();
int params_set_gain(uint8_t gain);
void params_set_samplerate();
int8_t params_get_port_avg_2pwr();

#endif
