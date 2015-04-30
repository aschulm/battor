#ifndef SAMPLES_H
#define SAMPLES_H

struct sample_
{
	int16_t signal;
} __attribute__((packed)); 
typedef struct sample_ sample;

struct samples_hdr_
{
	uint32_t seqnum;
	uint16_t samples_len;
} __attribute__((packed));
typedef struct samples_hdr_ samples_hdr;

void samples_init(uint16_t ovs_bits);
double sample_v(sample* s, double cal, uint8_t warning);
double sample_i(sample* s, double cal, double gain, uint8_t warning);
void samples_print_loop(double gain, double ovs_bits, char verb, uint32_t sample_rate);

#endif
