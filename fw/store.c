#include "common.h"

#include "store.h"
#include "blink.h"

static uint32_t write_block_pos = 0;
static uint32_t read_block_pos = 0;

uint8_t store_init() //{{{
{
	sd_info info;
	// check to see if there is an SD card
	if (sd_init(&info) < 0)
		return 0;

	return 1;
} //}}}

/*
while (startb.magic == STORE_MAGIC)
{
	startb_block += startb.blocks; // advance to next startblock
	if (past_dummy_block)
		memcpy(startb_prev, &startb, sizeof(startb)); // save last startblock
		past_dummy_block = 1;
		sd_read_block(&startb, startb_block);
}

uint32_t store_find_last_startblock(store_file_startblock* startb_prev) //{{{
{
	store_file_startblock startb;
	uint32_t startb_block = 0;
	uint8_t past_dummy_block = 0;
	memset(startb_prev, 0, sizeof(startb));

	// fill dummy start block
	memset(&startb, 0, sizeof(startb));
	startb.magic = STORE_MAGIC; 

	while (startb.magic == STORE_MAGIC)
	{
		startb_block += startb.blocks; // advance to next startblock
		if (past_dummy_block)
			memcpy(startb_prev, &startb, sizeof(startb)); // save last startblock
		past_dummy_block = 1;
		sd_read_block(&startb, startb_block);
	}

	return startb_block;
} //}}}

uint16_t store_create_next_file()
{
	uint32_t startb_free_block;
	store_file_startblock startb_prev;
	startb_first_free_block = store_find_last_startblock(&startb_prev);

	printf("Last startblock is at block %ld\n", startb_free_block, startb_prev.magic);
}

	// default fle size is 1 (startblock) (

	// find the last used file

	// erase the file
	//sd_erase_blocks(free_file_start_block, free_file_start_block + num_blocks);

	// set the file position
	//write_block_pos = free_file_start_block;

	// write the superblock
	//sd_write_block(&superb, STORE_SUPERBLOCK_NUM);
*/
