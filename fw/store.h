#ifndef STORE_H
#define STORE_H

#include "control.h"

#define STORE_STARTBLOCK_IDX 0
#define STORE_MAGIC 0xAAD5
#define STORE_VERSION 1
#define STORE_FILE_BLOCKS 2048000 // about 8 hours per file
#define STORE_MAX_FILES 1

typedef struct store_startblock_t
{
	uint16_t magic;
	uint16_t version;
	uint16_t rand;
} store_startblock;

typedef struct store_fileblock_t
{
	uint16_t len;
	uint16_t rand;
	uint8_t buf[508];
} store_fileblock;

int8_t store_init();
void store_write_open();
void store_write_bytes(uint8_t* buf, uint16_t len);
void store_read_open(uint16_t file);
int store_read_bytes(uint8_t* buf, uint16_t len);

#endif
