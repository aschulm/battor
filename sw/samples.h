#ifndef SAMPLES_H
#define SAMPLES_H

#define SAMPLES_DELIM0 0xBA 
#define SAMPLES_DELIM1 0x77

struct sample_
{
	int16_t signal;
} __attribute__((packed)); 
typedef struct sample_ sample;

struct samples_hdr_
{
	uint8_t seqnum;
	uint16_t samples_len;
} __attribute__((packed));
typedef struct samples_hdr_ samples_hdr;

float sample_v(sample* s);
float sample_v_gpio(sample* s, int *gpio_seen);
float sample_i(sample* s, float gain, float current_offset);
void samples_print_loop(float gain, float current_offset);

#endif
