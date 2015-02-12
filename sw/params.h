#ifndef PARAMS_H
#define PARAMS_H

#define BAUD_RATE 921600

#define ADC_MAX 0x7FF
#define VREF 1.0

// sample rate
#define CLOCK_HZ 16000000
#define TIMER_OVF_DEFAULT 250
#define TIMER_DIV XMEGA_CLKDIV_64_gc
#define SAMPLE_RATE_HZ_DEFAULT 1000  // paired with timer settings above

// current measurement
#define ADC_TOP 2047.0
#define AMPPOT_POS_DEFAULT 200
#define AMPPOT_OHM 50000.0 // base resistance of the amp potentiometer
#define CURRENT_GAIN_DEFAULT 5 // paired with gain settings above
#define IRES_OHM 0.18 // current sense resistor resistance
#define FILPOT_OHM 20000.0
#define FILCAP_F 0.0000001
#define FILPOT_POS 81 // with 0.1 uF cap corner is 1006.5 Hz, must correspond to the sample rate

#define BATTOR_UART_PATH_APPLE "/dev/cu.usbserial-A100QZQN"
#define BATTOR_UART_PATH_LINUX "/dev/ttyUSB0"

// voltage measurement
#define V_DEV (2.0/(2.0+10.0))

double param_sample_rate(uint32_t desired_sample_rate_hz, uint16_t* t_ovf, uint16_t* t_div, uint16_t* filpot_pos);
float param_gain(uint32_t desired_gain, uint16_t* amppot_pos);

#endif
