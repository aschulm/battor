#ifndef PARAMS_H
#define PARAMS_H

#define BAUD_RATE 2000000

#define VREF 1.200

// sample rate
#define CLOCK_HZ 32000000
#define TIMER_OVF_DEFAULT 250
#define SAMPLE_RATE_HZ_DEFAULT 1000  // paired with timer settings above

// gain
typedef enum PARAM_GAIN_enum
{
	PARAM_GAIN_LOW = 0,
	PARAM_GAIN_HIGH
} PARAM_GAIN_t;
#define GAIN_DEFAULT PARAM_GAIN_LOW

// adc params
#define ADC_BITS 11  // because it's a signed 12-bit ADC

// current measurement
#define AMPPOT_OHM 100000.0
#define FILPOT_OHM 100000.0
#define FILCAP_F 0.01e-6
#define FILPOT_POS 81 // with 0.1 uF cap corner is 1006.5 Hz, must correspond to the sample rate

// voltage measurement
#define V_DEV(r1,r2) (r2/(r1+r2))

// oversampling
#define OVERSAMPLE_BITS_DEFAULT 0
#define OVERSAMPLE_BITS_MAX 1

// control
#define CONTROL_SLEEP_US 50000

// uart
// needs at least the latency of the FTDI, so 20ms is plenty
#define UART_RX_ATTEMPTS 20
#define UART_READ_SLEEP_NS 1000000 

struct eeprom_params_
{
	uint8_t magic[4];       // Magic (0x31103110)
	uint16_t version;       // Version (0)
	uint32_t timestamp;     // Calibration timestamp (epoch time in secs)
	float R1;               // R1 [current sense] value
	float R2;               // R2 [V divider numerator] value
	float R3;               // R3 [V divider denominator] value
	float gainL;            // Low gain value
	float gainL_R1corr;     // Low gain R1 correction
	float gainL_U1off;      // Low gain U1 offset correction
	uint16_t gainL_amppot;  // Low gain amplifier potentiomter position
	float gainH;            // High gain value
	float gainH_R1corr;     // High gain R1 correction
	float gainH_U1off;      // High gain U1 offset correction
	uint16_t gainH_amppot;  // High gain amplifier potentiomter position
	uint32_t crc32;         // CRC32 [zip algorithm]
} __attribute__((packed));
typedef struct eeprom_params_ eeprom_params;

uint32_t param_sample_rate(uint32_t desired_sample_rate_hz, uint16_t ovs_bits, uint16_t* t_ovf, uint16_t* t_div, uint16_t* filpot_pos);
int param_read_eeprom(eeprom_params* params);

#endif
