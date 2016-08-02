#ifndef SAMPLES_H
#define SAMPLES_H

struct sample_
{
	int16_t v;
	int16_t i;
} __attribute__((packed)); 
typedef struct sample_ sample;

struct samples_hdr_
{
	uint32_t seqnum;
	uint16_t len;
} __attribute__((packed));
typedef struct samples_hdr_ samples_hdr;

struct samples_config_
{
	double gain;
	uint32_t sample_rate;
	eeprom_params eeparams;
	uint8_t ovs_bits;
	char cal;
};
typedef struct samples_config_ samples_config;

void samples_init(samples_config* conf);
void samples_print_config(samples_config* conf);
void samples_print_loop(samples_config* conf);

#endif
