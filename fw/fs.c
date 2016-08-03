#include "common.h"

#include "fs.h"

// temporary block used for reads and writes 
static uint8_t block[SD_BLOCK_LEN];
static int32_t block_idx;

// filesystem state
static uint32_t fs_fmt_iter;
static uint32_t fs_capacity;
static uint8_t fs_force_capacity = 0;

// open file state
static int32_t file_startblock_idx;
static uint32_t file_byte_idx;
static uint32_t file_byte_len;
static int32_t file_prev_skip_startblock_idx;
static uint32_t file_rtc_start_time_s;
static uint32_t file_rtc_start_time_ms;
// Current file sequence number
// * 1 if no file is open
// * current file seq if file is open
uint32_t g_fs_file_seq;

// write state
static uint8_t write_in_progress;
static uint8_t sd_write_in_progress;
void* write_buf;
uint16_t write_len;

// testing state
#ifdef FS_SKIP_TEST
static uint32_t fs_open_reads;
#endif

static uint8_t magic[8] = {0xBA, 0x77, 0xBA, 0x77, 0xBA, 0x77, 0xBA, 0x77};

static inline uint32_t min(uint32_t a, uint32_t b) //{{{
{
	return (a < b) ? a : b;
} //}}}

static inline uint32_t BYTES_TO_BLOCKS(uint32_t bytes) //{{{
{
	// assumes a block len of 512
	return (bytes >> 9) + ((bytes & 0x1FF) > 0);
} //}}}

static inline uint16_t BYTES_IN_LAST_BLOCK(uint32_t bytes) //{{{
{
	return (bytes & 0x1FF);
} //}}}

static void fs_init() //{{{
{
	fs_fmt_iter = 0;

	if (!fs_force_capacity)
		fs_capacity = sd_capacity();

	file_startblock_idx = -1;
	file_byte_idx = 0;
	file_byte_len = 0;
	file_rtc_start_time_s = 0;
	file_rtc_start_time_ms = 0;
	g_fs_file_seq = 1;
	file_prev_skip_startblock_idx = -1;

	block_idx = -1;

	write_buf = NULL;
	write_in_progress = 0;
	sd_write_in_progress = 0;

	memset(block, 0, sizeof(block));

#ifdef FS_SKIP_TEST
	fs_open_reads = 0;
#endif
} //}}}

int fs_info(fs_superblock* sb) //{{{
{
	if (sd_read_block(block, FS_SUPERBLOCK_IDX) < 0)
		return -1;

	memcpy(sb, block, sizeof(fs_superblock));

	// open last file to fill in current open file
	fs_open(0, 0);

	return 0;
} //}}}

int fs_format(uint8_t portable) //{{{
{
	int i;
	fs_superblock* sb = (fs_superblock*)block;

	// reset filesystem state
	fs_init();

	// read superblock
	block_idx = FS_SUPERBLOCK_IDX;
	if (!sd_read_block(block, block_idx))
		return FS_ERROR_SD_READ;

	// this sd card has been formatted before
	if (memcmp(sb->magic, magic, sizeof(magic)) == 0 &&
		sb->ver == FS_VERSION)
	{
		// format to the next format iteration
		sb->fmt_iter++;
	}
	// this sd card has not been formatted before, format it
	else
	{
		// create a superblock for the first iteration
		memcpy(sb->magic, magic, sizeof(magic));
		sb->ver = FS_VERSION;
		sb->fmt_iter = 0;
	}
	sb->portable = portable;
	fs_fmt_iter = sb->fmt_iter;

	// write the new superblock
	block_idx = FS_SUPERBLOCK_IDX;
	if (!sd_write_block_start(block, block_idx))
		return FS_ERROR_SD_WRITE;
	while (sd_write_block_update() < 0);

	// fill the first file block with ones so it is invalid
	for (i = 0; i < SD_BLOCK_LEN; i++)
		block[i] = 0xFF;
	if (!sd_write_block_start(block, block_idx + 1))
		return FS_ERROR_SD_WRITE;
	while (sd_write_block_update() < 0);

	return FS_ERROR_NONE;
} //}}}

int fs_open(uint8_t create_file, uint32_t file_seq_to_open) //{{{
{
	uint32_t file_byte_len_prev = -1;
	uint32_t file_rtc_start_time_s_prev = 0;
	uint32_t file_rtc_start_time_ms_prev = 0;
	int32_t block_idx_prev = -1;

	// reset filesystem state
	fs_init();

	// **********************
	// try to read superblock
	// **********************
	fs_superblock* sb = (fs_superblock*)block;
	block_idx = FS_SUPERBLOCK_IDX;
	if (!sd_read_block(block, block_idx))
		return FS_ERROR_SD_READ;
	// no superblock or corrupted
	if(!(memcmp(sb->magic, magic, sizeof(magic)) == 0 &&
		sb->ver == FS_VERSION))
	{
		if (create_file)
		{
			// format only happens this way if not in portable mode
			fs_format(0);
		}
	}
	// superblock is good, read iteration
	else
		fs_fmt_iter = sb->fmt_iter;

	// **************************
	// find the file to be opened
	// **************************
	while (block_idx < (fs_capacity - FS_LAST_FILE_BLOCKS))
	{
		// read in a potential file block
		fs_file_startblock* file = (fs_file_startblock*)block;
		block_idx++;
		if (!sd_read_block(block, block_idx))
			return FS_ERROR_SD_READ;
#ifdef FS_SKIP_TEST
			fs_open_reads++;
#endif

		// ---------------------
		// file startblock found
		// ---------------------
		if (file->fmt_iter == fs_fmt_iter && 
			file->seq == g_fs_file_seq)
		{
			// this is the requested file to open, so open it
			if (!create_file && file_seq_to_open > 0 &&
				file->seq == file_seq_to_open)
			{
				file_startblock_idx = block_idx;
				file_byte_len = file->byte_len;
				file_byte_idx = 0;
				file_rtc_start_time_s = file->rtc_start_time_s;
				file_rtc_start_time_ms = file->rtc_start_time_ms;
				return FS_ERROR_NONE;
			}

			// store previous skip block idx so it can be written with next skip block
			// mod 1 because file seq starts at 1
			if ((g_fs_file_seq % FS_FILE_SKIP_LEN) == 1)
				file_prev_skip_startblock_idx = block_idx;

			// at skip file - skip
			if (file->next_skip_file_startblock_idx > 0)
			{
				// no prev file due to skip
				block_idx_prev = -1;
				file_byte_len_prev = -1;
				file_rtc_start_time_s_prev = 0;
				file_rtc_start_time_ms_prev = 0;

				block_idx = file->next_skip_file_startblock_idx-1;
				g_fs_file_seq += FS_FILE_SKIP_LEN;
			}
			// at normal file - seek to the end
			else
			{
				block_idx_prev = block_idx;
				file_byte_len_prev = file->byte_len;
				file_rtc_start_time_s_prev = file->rtc_start_time_s;
				file_rtc_start_time_ms_prev = file->rtc_start_time_ms;

				block_idx += BYTES_TO_BLOCKS(file->byte_len);
				g_fs_file_seq++;
			}
		}
		// ------------------------
		// no file startblock found
		// ------------------------
		else
		{
			// opening new file
			if (create_file)
			{
				file_startblock_idx = block_idx;
				file_byte_idx = 0;
			}
			// opening existing file
			else
			{
				if (file_seq_to_open == 0 &&
					block_idx_prev >= 0)
				{
					file_startblock_idx = block_idx_prev;
					file_byte_len = file_byte_len_prev;
					file_byte_idx = 0;
					file_rtc_start_time_s = file_rtc_start_time_s_prev;
					file_rtc_start_time_ms = file_rtc_start_time_ms_prev;
				}
				else
					return FS_ERROR_NO_EXISTING_FILE;
			}
			return FS_ERROR_NONE;
		}
	}

	// *************************************************
	// the only free blocks are near the end of the disk
	// *************************************************
	if (block_idx >= (fs_capacity - FS_LAST_FILE_BLOCKS))
	{
		// format will only happen this way if not in portable mode
		// assumes that no one fills up the SD card in portable mode
		fs_format(0);
		// retry open
		return fs_open(create_file, file_seq_to_open);
	}

	return FS_ERROR_FILE_TOO_LONG;
} //}}}

int fs_rtc_get(uint32_t* s, uint32_t* ms) //{{{
{
	// fail if there is no open file
	if (file_startblock_idx < 0)
		return -1;

	// return the rtc time for the currently opened file
	*s = file_rtc_start_time_s;
	*ms = file_rtc_start_time_ms;
	return 0;
} //}}}

int fs_rtc_set() //{{{
{
	uint32_t s, ms;
	// fail if there is no open file
	if (file_startblock_idx < 0)
		return -1;

	// set the rtc time for the currently opened file
	timer_rtc_get(&s, &ms);
	file_rtc_start_time_s = s;
	file_rtc_start_time_ms = ms;
	return 0;
} //}}}

int fs_close() //{{{
{
	fs_file_startblock* file = NULL; 

	if (file_startblock_idx < 0)
		return FS_ERROR_FILE_NOT_OPEN;

	// finish updating the file
	while(fs_busy())
		fs_update();

	// there is one more partial block that remains to be written
	uint32_t last_block_idx = file_startblock_idx + BYTES_TO_BLOCKS(file_byte_idx);
	if (last_block_idx != block_idx)
	{
		if (last_block_idx >= fs_capacity)
			return FS_ERROR_FILE_TOO_LONG;
		sd_write_block_start(block, last_block_idx);
		while (sd_write_block_update() < 0);
	}

	file = (fs_file_startblock*)block;
	memset(block, 0, sizeof(block));
	file->seq = g_fs_file_seq;
	file->byte_len = file_byte_idx;
	file->fmt_iter = fs_fmt_iter;
	file->rtc_start_time_s = file_rtc_start_time_s;
	file->rtc_start_time_ms = file_rtc_start_time_ms;

	// write file block
	if (file_startblock_idx >= fs_capacity)
			return FS_ERROR_FILE_TOO_LONG;
	if (!sd_write_block_start(block, file_startblock_idx))
		return FS_ERROR_SD_WRITE;
	while (sd_write_block_update() < 0);

	// this is a skip file, point to it from the previous skip file
	// mod 1 because file seq starts at 1
	if ((g_fs_file_seq % FS_FILE_SKIP_LEN) == 1 &&
	    file_prev_skip_startblock_idx > 0)
	{
		// read the prev skip file startblock
		fs_file_startblock* skip_file = (fs_file_startblock*)block;
		memset(block, 0, sizeof(block));
		if (!sd_read_block(block, file_prev_skip_startblock_idx))
			return FS_ERROR_SD_READ;

		// write prev skip file startblock with the new skip pointer
		skip_file->next_skip_file_startblock_idx = file_startblock_idx;
		if (!sd_write_block_start(block, file_prev_skip_startblock_idx))
			return FS_ERROR_SD_WRITE;
		while (sd_write_block_update() < 0);
	}

	// reset filesystem state
	fs_init();

	return FS_ERROR_NONE;
} //}}}

int fs_write(void* buf, uint16_t len) //{{{
{
	// TODO WHAT IF WRITING TO THE END OF THE SD CARD!
	
	if (file_startblock_idx < 0)
		return FS_ERROR_FILE_NOT_OPEN;

	if (fs_busy())
		return FS_ERROR_WRITE_IN_PROGRESS;

	// note: write_buf must exist until fs_busy() returns 0
	write_buf = buf;
	write_len = len;
	write_in_progress = 1;
	sd_write_in_progress = 0;

	return FS_ERROR_NONE;
} //}}}

int fs_read(void* buf, uint16_t len) //{{{
{
	uint16_t read = 0;

	if (file_startblock_idx < 0)
		return FS_ERROR_FILE_NOT_OPEN;

	if (file_byte_idx >= file_byte_len)
		return FS_ERROR_EOF;

	// TODO add a check to make sure not trying to read from a new file

	// read until end of requested len or end of file
	while ((read < len) && (file_byte_idx < file_byte_len))
	{
		uint16_t block_byte_idx = BYTES_IN_LAST_BLOCK(file_byte_idx);
		uint16_t read_remaining = min(len - read, file_byte_len - file_byte_idx);

		// can only read at most a block at a time
		uint16_t to_read = min(read_remaining, SD_BLOCK_LEN - block_byte_idx);

		// next block needs to be read from sd
		if (file_startblock_idx +
			BYTES_TO_BLOCKS(file_byte_idx + to_read) != block_idx)
		{
			block_idx = file_startblock_idx + 
				BYTES_TO_BLOCKS(file_byte_idx + to_read);
			sd_read_block(block, block_idx);
		}

		memcpy(buf + read, block + block_byte_idx, to_read);
		read += to_read;
		file_byte_idx += to_read;
	}

	return read;
} //}}}

void fs_update() //{{{
{
	// update the sd write if it is in progress
	if (sd_write_in_progress)
		sd_write_in_progress = (sd_write_block_update() < 0);
	else
	{
		uint16_t block_byte_idx = BYTES_IN_LAST_BLOCK(file_byte_idx);
		uint16_t block_remaining = SD_BLOCK_LEN - block_byte_idx;
		uint16_t to_write = min(write_len, block_remaining);

		memcpy(block + block_byte_idx, write_buf, to_write);
		write_buf += to_write;
		write_len -= to_write;
		file_byte_idx += to_write;

		// completed block --- write it to SD
		if (block_byte_idx + to_write == SD_BLOCK_LEN)
		{
			uint32_t block_idx_to_write = file_startblock_idx + BYTES_TO_BLOCKS(file_byte_idx);
			if (block_idx_to_write >= fs_capacity)
				return;
			sd_write_block_start(block, block_idx_to_write);
			sd_write_in_progress = 1;
		}

		// there is no more data left to write --- write is done
		if (write_len == 0)
			write_in_progress = 0;
	}
} //}}}

int fs_busy() //{{{
{
	return (write_in_progress || sd_write_in_progress);
} //}}}

int fs_self_test() //{{{
{
	uint8_t buf[SD_BLOCK_LEN], buf2[SD_BLOCK_LEN];
	uint32_t fmt_iter;
	int ret, i;

	printf("fs self test\n");

	// start with clean fs and
	// remember fs_fmt_iter so it can be read after fs_close()
	fs_format(0);
	fmt_iter = fs_fmt_iter;

	printf("fs_open new file with no existing file and do not close...");
	if ((ret = fs_open(1, 0)) < 0)
	{
		printf("FAILED fs_open failed, returned %d\n", ret);
		return 1;
	}
	if (file_startblock_idx != 1)
	{
		printf("FAILED first new file startblock is not block 1, it's %ld\n", file_startblock_idx);
		return 1;
	}
	printf("PASSED\n");

	printf("fs_open existing file with no existing file...");
	if ((ret = fs_open(0, 0)) >= 0)
	{
		printf("FAILED fs_open did not fail even though it should\n");
		return 1;
	}
	printf("PASSED\n");

	printf("fs_open new file and fs_close...");
	if ((ret = fs_open(1, 0)) < 0)
	{
		printf("FAILED fs_open failed, returned %d\n", ret);
		return 1;
	}
	if ((ret = fs_close()) < 0)
	{
		printf("FAILED fs_close failed, returned %d\n", ret);
		return 1;
	}
	// check file block
	fs_file_startblock file1;
	sd_read_block(block, 1);
	file1.seq = 1;
	file1.byte_len = 0;
	file1.fmt_iter = fmt_iter;
	file1.next_skip_file_startblock_idx = 0;
	if (memcmp(&file1, block, sizeof(file1)) != 0)
	{
		printf("FAILED file block is incorrect\n");
		return 1;
	}
	printf("PASSED\n");

	printf("fs_open another new file and fs_close...");
	if ((ret = fs_open(1, 0)) < 0)
	{
		printf("FAILED fs_open failed, returned %d\n", ret);
		return 1;
	}
	if ((ret = fs_close()) < 0)
	{
		printf("FAILED fs_close failed, returned %d\n", ret);
		return 1;
	}
	// check file block
	fs_file_startblock file2;
	sd_read_block(block, 2);
	file2.seq = 2;
	file2.byte_len = 0;
	file2.fmt_iter = fmt_iter;
	file2.next_skip_file_startblock_idx = 0;

	if (memcmp(&file2, block, sizeof(file2)) != 0)
	{
		printf("FAILED file block is incorrect\n");
		return 1;
	}
	printf("PASSED\n");

	printf("fs_open a new file, write less than a block, and fs_close...");
	if ((ret = fs_open(1, 0)) < 0)
	{
		printf("FAILED fs_open failed, returned %d\n", ret);
		return 1;
	}
	// write 10 bytes
	for (i = 0; i < 10; i++)
		buf[i] = i;
	fs_write(buf, 10);
	if ((ret = fs_close()) < 0)
	{
		printf("FAILED fs_close failed, returned %d\n", ret);
		return 1;
	}
	// check file block
	fs_file_startblock file3;
	sd_read_block(block, 3);
	file3.fmt_iter = fmt_iter;
	file3.seq = 3;
	file3.byte_len = 10;
	file3.next_skip_file_startblock_idx = 0;

	if (memcmp(&file3, block, sizeof(file3)) != 0)
	{
		printf("FAILED file block is incorrect\n");
		return 1;
	}
	// check data
	sd_read_block(block, 4);
	if (memcmp(buf, block, 10) != 0)
	{
		printf("FAILED file data is incorrect\n");
		return 1;
	}
	printf("PASSED\n");

	printf("fs_open a new file, write more than a block, and fs_close...");
	if ((ret = fs_open(1, 0)) < 0)
	{
		printf("FAILED fs_open failed, returned %d\n", ret);
		return 1;
	}
	memset(buf, (uint8_t)fmt_iter, sizeof(buf));
	// write 500 bytes
	fs_write(buf, 500);
	while (fs_busy())
		fs_update();
	memset(buf, (uint8_t)fmt_iter + 1, sizeof(buf));
	// write 14 bytes
	fs_write(buf, 14);
	if ((ret = fs_close()) < 0)
	{
		printf("FAILED fs_close failed, returned %d\n", ret);
		return 1;
	}
	// check file block
	fs_file_startblock file4;
	sd_read_block(block, 5);
	file4.fmt_iter = fmt_iter;
	file4.seq = 4;
	file4.byte_len = 514;
	file4.next_skip_file_startblock_idx = 0;

	if (memcmp(&file4, block, sizeof(file3)) != 0)
	{
		printf("FAILED file block is incorrect\n");
		return 1;
	}
	// check first block
	memset(buf, (uint8_t)fmt_iter, sizeof(buf));
	sd_read_block(block, 6);
	if (memcmp(buf, block, 500) != 0)
	{
		printf("FAILED file data is incorrect #1\n");
		return 1;
	}
	memset(buf, (uint8_t)fmt_iter + 1, sizeof(buf));
	if (memcmp(buf, block + 500, 12) != 0)
	{
		printf("FAILED file data is incorrect #2\n");
		return 1;
	}
	// check second block 
	sd_read_block(block, 7);
	if (memcmp(buf, block, 2) != 0)
	{
		printf("FAILED file data is incorrect #3\n");
		return 1;
	}
	printf("PASSED\n");

	printf("fs_open an existing file, fs_read more than a block...");
	if ((ret = fs_open(0, 0)) < 0)
	{
		printf("FAILED fs_open failed, returned %d\n", ret);
		return 1;
	}
	fs_read(buf2, 500);
	memset(buf, (uint8_t)fmt_iter, sizeof(buf));
	if (memcmp(buf, buf2, 500) != 0)
	{
		printf("FAILED file data is incorrect #1\n");
		return 1;
	}
	fs_read(buf2, 14);
	memset(buf, (uint8_t)fmt_iter + 1, sizeof(buf));
	if (memcmp(buf, buf2, 14) != 0)
	{
		printf("FAILED file data is incorrect #2\n");
		return 1;
	}
	if (fs_read(buf2, 1) != FS_ERROR_EOF)
	{
		printf("FAILED fs_read should have failed with EOF\n");
		return 1;
	}
	printf("PASSED\n");

	printf("fs_open a new file, big write, big read...");
	if ((ret = fs_open(1, 0)) < 0)
	{
		printf("FAILED fs_open failed, returned %d\n", ret);
		return 1;
	}
	// init write/read buffer
	for (i = 0; i < 200; i++)
		buf[i] = i;
	// write many times
	for (i = 0; i < 10; i++)
	{
		fs_write(buf, 200);
		while (fs_busy())
			fs_update();
	}
	if ((ret = fs_close()) < 0)
	{
		printf("FAILED fs_close failed, returned %d\n", ret);
		return 1;
	}
	if ((ret = fs_open(0, 0)) < 0)
	{
		printf("FAILED fs_open failed, returned %d\n", ret);
		return 1;
	}
	// read many times
	for (i = 0; i < 10; i++)
	{
		fs_read(buf2, 200);
		if (memcmp(buf, buf2, 200) != 0)
		{
			printf("FAILED file data is incorrect in iteration %d\n", i);
			return 1;
		}
	}
	printf("PASSED\n");

	// start with clean fs to see if reformat happens correctly
	// remember fs_fmt_iter so it can be read after fs_close()
	fs_format(0);
	fmt_iter = fs_fmt_iter;
	fs_capacity = FS_LAST_FILE_BLOCKS + 2;
	fs_force_capacity = 1;

	printf("fs_open file near the end of the disk...");
	if ((ret = fs_open(1, 0)) < 0)
	{
		printf("FAILED fs_open failed, returned %d\n", ret);
		return 1;
	}
	fs_write(buf, 10); // write one more block
	if ((ret = fs_close()) < 0)
	{
		printf("FAILED fs_close failed, returned %d\n", ret);
		return 1;
	}
	// check file block
	fs_file_startblock file5;
	sd_read_block(block, 1);
	file5.fmt_iter = fmt_iter;
	file5.seq = 1;
	file5.byte_len = 10;
	file5.next_skip_file_startblock_idx = 0;
	if (memcmp(&file5, block, sizeof(file5)) != 0)
	{
		printf("FAILED file block is incorrect\n");
		return 1;
	}
	printf("PASSED\n");

	printf("fs_open file past the end of the last blocks on the disk...");
	if ((ret = fs_open(1, 0)) < 0)
	{
		printf("FAILED fs_open failed, returned %d\n", ret);
		return 1;
	}
	fs_write(buf, 10); // write one more block
	if ((ret = fs_close()) < 0)
	{
		printf("FAILED fs_close failed, returned %d\n", ret);
		return 1;
	}
	// check file block
	fs_file_startblock file6;
	sd_read_block(block, 1);
	file6.fmt_iter = fmt_iter+1;
	file6.seq = 1;
	file6.byte_len = 10;
	file6.next_skip_file_startblock_idx = 0;
	if (memcmp(&file6, block, sizeof(file6)) != 0)
	{
		printf("FAILED file block is incorrect\n");
		return 1;
	}
	printf("PASSED\n");

	fs_force_capacity = 0;

#ifdef FS_SKIP_TEST
	fs_file_startblock *file = (fs_file_startblock*)block;

	// start with clean fs so counting skip blocks is possible and
	// remember fs_fmt_iter so it can be read after fs_close()
	fs_format(0);
	fmt_iter = fs_fmt_iter;

	printf("fs_open and fs_close %d files to check skip...", FS_FILE_SKIP_LEN+1);
	for (i = 0; i < (FS_FILE_SKIP_LEN+1); i++)
	{
		if ((ret = fs_open(1, 0)) < 0)
		{
			printf("FAILED fs_open failed, returned %d\n", ret);
			return 1;
		}
		if ((ret = fs_close()) < 0)
		{
			printf("FAILED fs_close failed, returned %d\n", ret);
			return 1;
		}
	}
	// check file block
	fs_file_startblock file_skip1;
	file_skip1.fmt_iter = fmt_iter;
	file_skip1.seq = 1;
	file_skip1.byte_len = 0;
	file_skip1.next_skip_file_startblock_idx = 251;
	sd_read_block(block, 1);
	if (memcmp(&file_skip1, block, sizeof(file_skip1)) != 0)
	{
		printf("FAILED file skip block is not idx 251, it's %lu\n",
			file->next_skip_file_startblock_idx);
		return 1;
	}
	printf("PASSED\n");

	printf("check if fs_open new uses skip...");
	if ((ret = fs_open(1, 0)) < 0)
	{
		printf("FAILED fs_open failed, returned %d\n", ret);
		return 1;
	}
	if (fs_open_reads != 3)
	{
		printf("FAILED should be 3 iterations to open a file with a skip block, was %ld\n", fs_open_reads);
		return 1;
	}
	if (file_startblock_idx != 252)
	{
		printf("FAILED new file is not one after skip block\n");
		return 1;
	}
	if ((ret = fs_close()) < 0)
	{
		printf("FAILED fs_close failed, returned %d\n", ret);
		return 1;
	}
	printf("PASSED\n");

	printf("check if fs_open existing uses skip...");
	if ((ret = fs_open(0, 0)) < 0)
	{
		printf("FAILED fs_open failed, returned %d\n", ret);
		return 1;
	}
	if (fs_open_reads != 4)
	{
		printf("FAILED should be 4 iterations to open a file with a skip block, was %ld\n", fs_open_reads);
		return 1;
	}
	if (file_startblock_idx != 252)
	{
		printf("FAILED new file is not one after skip block\n");
		return 1;
	}
	if ((ret = fs_close()) < 0)
	{
		printf("FAILED fs_close failed, returned %d\n", ret);
		return 1;
	}
	printf("PASSED\n");

	printf("fs_open, fs_write, fs_close %d files to check 2nd skip...", FS_FILE_SKIP_LEN+1);
	for (i = 0; i < 251; i++)
	{
		if ((ret = fs_open(1, 0)) < 0)
		{
			printf("FAILED fs_open failed, returned %d\n", ret);
			return 1;
		}
		fs_write(buf, 200);
		if ((ret = fs_close()) < 0)
		{
			printf("FAILED fs_close failed, returned %d\n", ret);
			return 1;
		}
	}
	// check file block
	fs_file_startblock file_skip2;
	file_skip2.fmt_iter = fmt_iter;
	file_skip2.seq = 251;
	file_skip2.byte_len = 0;
	file_skip2.next_skip_file_startblock_idx = 750;
	sd_read_block(block, 251);
	if (memcmp(&file_skip2, block, sizeof(file_skip2)) != 0)
	{
		printf("FAILED file skip block is not idx 750, it's %lu\n",
			file->next_skip_file_startblock_idx);
		return 1;
	}
	printf("PASSED\n");
#endif

	return 0;
} //}}}
