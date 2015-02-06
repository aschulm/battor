#ifndef PARAMS_H
#define PARAMS_H

#define BAUD_RATE 800000

#define VREF 2.048

// sample rate
#define CLOCK_HZ 32000000
#define TIMER_OVF_DEFAULT 250
#define SAMPLE_RATE_HZ_DEFAULT 1000  // paired with timer settings above

// adc params
#define ADC_BITS 11  // because it's a signed 12-bit ADC

// current measurement
#define AMPPOT_POS_DEFAULT 200
#define AMPPOT_OHM 100000.0
#define CURRENT_GAIN_DEFAULT 5 // paired with gain settings above
#define IRES_OHM 0.18 // current sense resistor resistance
#define FILPOT_OHM 100000.0
#define FILCAP_F 0.0000001
#define FILPOT_POS 81 // with 0.1 uF cap corner is 1006.5 Hz, must correspond to the sample rate

// voltage measurement
#define V_DEV (5.0/(5.0+10.0))

// oversampling
#define OVERSAMPLE_BITS_DEFAULT 1
#define OVERSAMPLE_BITS_MAX 1

uint32_t param_sample_rate(uint32_t desired_sample_rate_hz, uint16_t ovs_bits, uint16_t* t_ovf, uint16_t* t_div, uint16_t* filpot_pos);
double param_gain(uint32_t desired_gain, uint16_t* amppot_pos);

#endif
