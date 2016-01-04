#include "common.h"

#include "fs.h"

// temporary block used for reads and writes 
static uint8_t block[SD_BLOCK_LEN];
static int32_t block_idx;

// filesystem state
static uint32_t fs_fmt_iter;

// open file state
static int32_t file_startblock_idx;
static uint32_t file_byte_idx;
static uint32_t file_byte_len;
static uint32_t file_seq;

// write state
static uint8_t write_in_progress;
static uint8_t sd_write_in_progress;
void* write_buf;
uint16_t write_len;

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
	file_startblock_idx = -1;
	file_byte_idx = 0;
	file_byte_len = 0;
	file_seq = 0;

	block_idx = -1;

	write_buf = NULL;
	write_in_progress = 0;
	sd_write_in_progress = 0;

	memset(block, 0, sizeof(block));
} //}}}

static int format() //{{{
{
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
		fs_fmt_iter = sb->fmt_iter;
	}
	// this sd card has not been formatted before, format it
	else
	{
		// create a superblock for the first iteration
		memcpy(sb->magic, magic, sizeof(magic));
		sb->ver = FS_VERSION;
		sb->fmt_iter = 0;
		fs_fmt_iter = sb->fmt_iter;
	}

	// write the new superblock
	block_idx = FS_SUPERBLOCK_IDX;
	if (!sd_write_block_start(block, block_idx))
		return FS_ERROR_SD_WRITE;
	while (sd_write_block_update() < 0);

	return FS_ERROR_NONE;
} //}}}

int fs_open(uint8_t new_file) //{{{
{
	uint32_t capacity = sd_capacity();
	uint32_t file_byte_len_prev = -1;
	int32_t block_idx_prev = -1;

	// reset filesystem state
	fs_init();

	// read superblock
	fs_superblock* sb = (fs_superblock*)block;
	block_idx = FS_SUPERBLOCK_IDX;
	if (!sd_read_block(block, block_idx))
		return FS_ERROR_SD_READ;
	// no superblock or corrupted, format
	if (!(memcmp(sb->magic, magic, sizeof(magic)) == 0 &&
		sb->ver == FS_VERSION))
		format();
	// superblock is good, read iteration
	else
		fs_fmt_iter = sb->fmt_iter;

	// find the first free file
	while (block_idx < capacity)
	{
		// read in a potential file block
		fs_file_startblock* file = (fs_file_startblock*)block;
		block_idx++;
		if (!sd_read_block(block, block_idx))
			return FS_ERROR_SD_READ;

		// file startblock found - skip to end of file
		if (file->fmt_iter == fs_fmt_iter && 
			file->seq == file_seq)
		{
			block_idx_prev = block_idx;
			file_byte_len_prev = file->byte_len;
			// skip over file data
			block_idx += BYTES_TO_BLOCKS(file->byte_len);

			file_seq++; // increment current file sequence number
		}
		else
		{
			// opening new file
			if (new_file)
			{
				file_startblock_idx = block_idx;
				file_byte_idx = 0;
			}
			// opening existing file
			else
			{
				if (block_idx_prev >= 0)
				{
					file_startblock_idx = block_idx_prev;
					file_byte_len = file_byte_len_prev;
					file_byte_idx = 0;
				}
				else
					return FS_ERROR_NO_EXISTING_FILE;
			}
			return FS_ERROR_NONE;
		}
	}

	return FS_ERROR_FILE_TOO_LONG;
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
	if (file_startblock_idx + BYTES_TO_BLOCKS(file_byte_idx) != block_idx)
	{
		sd_write_block_start(block, file_startblock_idx + BYTES_TO_BLOCKS(file_byte_idx));
		while (sd_write_block_update() < 0);
	}

	file = (fs_file_startblock*)block;
	memset(block, 0, sizeof(block));
	file->seq = file_seq;
	file->byte_len = file_byte_idx;
	file->fmt_iter = fs_fmt_iter;

	// write file block
	if (!sd_write_block_start(block, file_startblock_idx))
		return FS_ERROR_SD_WRITE;
	while (sd_write_block_update() < 0);

	// reset filesystem state
	fs_init();

	return FS_ERROR_NONE;
} //}}}

int fs_write(void* buf, uint16_t len) //{{{
{
	// TODO WHAT IF WRITING TO THE END OF THE SD CARD!
	
	if (file_startblock_idx < 0)
		return FS_ERROR_FILE_NOT_OPEN;

	if (write_in_progress)
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
		sd_write_block_start(block, file_startblock_idx + BYTES_TO_BLOCKS(file_byte_idx));
		sd_write_in_progress = 1;
	}

	// there is no more data left to write --- write is done
	if (write_len == 0)
		write_in_progress = 0;
} //}}}

int fs_busy() //{{{
{
	return (write_in_progress || sd_write_in_progress);
} //}}}

int fs_self_test() //{{{
{
	uint8_t buf[SD_BLOCK_LEN], buf2[SD_BLOCK_LEN];
	int ret, i;
	uint32_t fmt_iter;
	printf("fs self test\n");

	// always format so self test has consistant filesystem state
	format();
	// remember the iteration so it can be checked after fs_close()
	fmt_iter = fs_fmt_iter;

	printf("fs_open new file with no existing file...");
	if ((ret = fs_open(1)) < 0)
	{
		printf("FAILED fs_open failed, returned %d\n", ret);
		return 1;
	}
	if (file_startblock_idx != 1)
	{
		printf("FAILED new file startblock is not block 1\n");
		return 1;
	}
	printf("PASSED\n");

	printf("fs_open existing file with no existing file...");
	if ((ret = fs_open(0)) >= 0)
	{
		printf("FAILED fs_open did not fail even though it should\n");
		return 1;
	}
	printf("PASSED\n");

	printf("fs_open new file and fs_close...");
	if ((ret = fs_open(1)) < 0)
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
	file1.seq = 0;
	file1.byte_len = 0;
	file1.fmt_iter = fmt_iter;
	if (memcmp(&file1, block, sizeof(file1)) != 0)
	{
		printf("FAILED file block is incorrect\n");
		return 1;
	}
	printf("PASSED\n");

	printf("fs_open another new file and fs_close...");
	if ((ret = fs_open(1)) < 0)
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
	file2.fmt_iter = fmt_iter;
	file2.seq = 1;
	file2.byte_len = 0;

	if (memcmp(&file2, block, sizeof(file2)) != 0)
	{
		printf("FAILED file block is incorrect\n");
		return 1;
	}
	printf("PASSED\n");

	printf("fs_open a new file, write less than a block, and fs_close...");
	if ((ret = fs_open(1)) < 0)
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
	file3.seq = 2;
	file3.byte_len = 10;
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
	if ((ret = fs_open(1)) < 0)
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
	file4.seq = 3;
	file4.byte_len = 514;
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
	if ((ret = fs_open(0)) < 0)
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

	return 0;
} //}}}
