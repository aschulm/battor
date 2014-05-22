#ifndef STORE_H
#define STORE_H

#define STORE_MAGIC 0xAAD5
#define STORE_VERSION 1

uint8_t store_init();

typedef struct store_file_startblock_t
{
	uint16_t magic;
	uint16_t version;
	uint32_t blocks;
	uint16_t written;
	uint8_t commands[502];
} store_file_startblock;

uint32_t store_find_last_startblock(store_file_startblock* startb_prev);

#endif
