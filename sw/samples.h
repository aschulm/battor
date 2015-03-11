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
	uint32_t seqnum;
	uint16_t samples_len;
} __attribute__((packed));
typedef struct samples_hdr_ samples_hdr;

void samples_init(uint16_t ovs_bits);
double sample_v(sample* s);
double sample_i(sample* s, double gain, double current_offset);
void samples_print_loop(double gain, double current_offset, double ovs_bits, char verb, uint32_t sample_rate);

#endif
