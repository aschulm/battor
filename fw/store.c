#include "common.h"

#include "blink.h"
#include "control.h"
#include "store.h"

static uint32_t write_block_idx = 0;
static uint32_t read_block_idx = 0;
static store_startblock sb;
static store_fileblock fb;

static void startblock_init(uint16_t rand) //{{{
{
	memset(&sb, 0, sizeof(store_startblock));
	sb.magic = STORE_MAGIC;
	sb.version = STORE_VERSION;
	sb.rand = rand;
} //}}}

int8_t store_init() //{{{
{
	sd_info info;
	// check to see if there is an SD card, if so init it
	if (sd_init(&info) < 0)
		return -1;

	// look for startblock, if not create it
	memset(&sb, 0, sizeof(store_startblock));
	sd_read_block(&sb, STORE_STARTBLOCK_IDX);
	if (sb.magic == STORE_MAGIC && sb.version == STORE_VERSION)
	{
		printf("store: startblock found\n");	
	}
	else
	{
		printf("store: startblock not found\n");	
		startblock_init(0);
		sd_write_block(&sb, STORE_STARTBLOCK_IDX);
	}

	return 0;
} //}}}

void store_write_open() //{{{
{
	uint32_t rand = 0;
	// get random int for start block number
	
	// set the current block index to the current file
	write_block_idx = rand * STORE_FILE_BLOCKS;
} //}}}

void store_write_bytes(uint8_t* buf, uint16_t len) //{{{
{
	uint16_t idx = 0;
	memset(&fb, 0, sizeof(store_fileblock));

	fb.rand = sb.rand;
	
	while (idx < len)
	{
		fb.len = ((len - idx) > sizeof(fb.buf)) ? sizeof(fb.buf) : (len - idx);
		memcpy(fb.buf, buf + idx, fb.len);
		sd_write_block(&fb, write_block_idx);

		write_block_idx++;
		idx += fb.len;
	}
} //}}}

void store_read_open(uint16_t file) //{{{
{
	read_block_idx = file * STORE_FILE_BLOCKS;
} //}}}

int store_read_bytes(uint8_t* buf, uint16_t len) //{{{
{
	uint16_t idx = 0;

	// assumes the same size that was written is also read
	sd_read_block(&fb, read_block_idx);
	printf("store: #1 rands sb %X fb %X\n", sb.rand, fb.rand);
	while (sb.rand == fb.rand && idx < len)
	{
		printf("store: rands sb %X fb %X fb.len %d idx %d len %d\n", sb.rand, fb.rand, fb.len, idx, len);
		memcpy(buf + idx, fb.buf, fb.len);
		idx += fb.len;
		read_block_idx++;
		sd_read_block(&fb, read_block_idx);
	}

	if (idx < len)
		return -1;

	return 0;
} //}}}
