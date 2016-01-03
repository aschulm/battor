#include "common.h"

#include "fs.h"

// temporary block used for reads and writes 
static uint8_t block[SD_BLOCK_LEN];
static uint16_t block_idx;

// filesystem state
static uint32_t fs_fmt_iter;

// current file state
static int32_t file_idx;
static uint32_t file_seq;
static uint32_t file_len;

// write state
static uint8_t write_in_progress;
static uint8_t sd_write_in_progress;
void* write_buf;
uint16_t write_len;

static uint8_t magic[8] = {0xBA, 0x77, 0xBA, 0x77, 0xBA, 0x77, 0xBA, 0x77};

static void fs_init() //{{{
{
	fs_fmt_iter = 0;
	block_idx = 0;
	file_idx = -1;
	file_seq = 0;
	file_len = 0;

	write_buf = NULL;
	write_in_progress = 0;
	sd_write_in_progress = 0;

	block_idx = 0;
	memset(block, 0, sizeof(block));

	// TODO read in number of blocks
} //}}}

static int format() //{{{
{
	fs_superblock* sb = (fs_superblock*)block;

	// reset filesystem state
	fs_init();

	// read superblock
	if (!sd_read_block(block, FS_SUPERBLOCK_IDX))
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
	if (!sd_write_block_start(block, FS_SUPERBLOCK_IDX))
		return FS_ERROR_SD_WRITE;
	while (sd_write_block_update() < 0);

	return FS_ERROR_NONE;
} //}}}

int fs_open(uint8_t new_file) //{{{
{
	uint32_t idx = 1; // start at first block after superblock
	int32_t idx_prev = -1;

	// reset filesystem state
	fs_init();

	// read superblock
	fs_superblock* sb = (fs_superblock*)block;
	if (!sd_read_block(block, FS_SUPERBLOCK_IDX))
		return FS_ERROR_SD_READ;
	// no superblock or corrupted, format
	if (!(memcmp(sb->magic, magic, sizeof(magic)) == 0 &&
		sb->ver == FS_VERSION))
		format();
	// superblock is good, read iteration
	else
	{
		fs_fmt_iter = sb->fmt_iter;
	}

	// find the first free file
	// TODO put the maximum block here
	while (idx < 100)
	{
		fs_file* file = (fs_file*)block;
		if (!sd_read_block(block, idx))
			return FS_ERROR_SD_READ;

		// file found - skip to end of file
		if (file->fmt_iter == fs_fmt_iter && 
			file->seq == file_seq)
		{
			idx_prev = idx;
			idx += file->len + 1; // skip the file block and the data

			file_seq++; // increment current file sequence number
		}
		else
		{
			if (new_file)
			{
				file_idx = idx;
			}
			else
			{
				if (idx_prev >= 0)
					file_idx = idx_prev;
				else
				{
					return FS_ERROR_NO_EXISTING_FILE;
				}
			}
			return FS_ERROR_NONE;
		}
	}

	return FS_ERROR_FILE_TOO_LONG;
} //}}}

int fs_close() //{{{
{
	fs_file* file = NULL; 
	if (file_idx < 0)
		return FS_ERROR_FILE_NOT_OPEN;

	// finish updating the file
	while(fs_busy())
		fs_update();

	// there is data that remains to be written, write it to sd
	if (block_idx > 0)
	{
		file_len++;
		sd_write_block_start(block, file_idx + file_len);
		while (sd_write_block_update() < 0);
	}

	file = (fs_file*)block;
	memset(block, 0, sizeof(block));
	file->seq = file_seq;
	file->len = file_len;
	file->fmt_iter = fs_fmt_iter;

	// write file block
	if (!sd_write_block_start(block, file_idx))
		return FS_ERROR_SD_WRITE;
	while (sd_write_block_update() < 0);

	// reset filesystem state
	fs_init();

	return FS_ERROR_NONE;
} //}}}

int fs_write(void* buf, uint16_t len) //{{{
{
	if (file_idx < 0)
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

void fs_read()
{
}

void fs_update() //{{{
{
	// update the sd write if it is in progress
	if (sd_write_in_progress)
		sd_write_in_progress = (sd_write_block_update() < 0);

	// write is short enough to fit into block storage
	if ((write_len + block_idx) < SD_BLOCK_LEN)
	{
		memcpy(block + block_idx, write_buf, write_len);
		block_idx += write_len;
		write_len = 0;
	}
	// write is larger or equal to block length, write it to sd
	else
	{
		uint16_t to_write_len = SD_BLOCK_LEN - block_idx;
		memcpy(block + block_idx, write_buf, to_write_len);
		block_idx = 0; // wrote block so reset index into it
		write_len -= to_write_len;
		write_buf += to_write_len;

		file_len++;
		sd_write_block_start(block, file_idx + file_len);
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
	uint8_t buf[SD_BLOCK_LEN];
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
	if (file_idx != 1)
	{
		printf("FAILED new file is not block 1\n");
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
	fs_file file1;
	sd_read_block(block, 1);
	file1.seq = 0;
	file1.len = 0;
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
	fs_file file2;
	sd_read_block(block, 2);
	file2.fmt_iter = fmt_iter;
	file2.seq = 1;
	file2.len = 0;
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
	fs_file file3;
	sd_read_block(block, 3);
	file3.fmt_iter = fmt_iter;
	file3.seq = 2;
	file3.len = 1;
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
	memset(buf, 21, sizeof(buf));
	// write 500 bytes
	fs_write(buf, 500);
	while (fs_busy())
		fs_update();
	memset(buf, 22, sizeof(buf));
	// write 14 bytes
	fs_write(buf, 14);
	if ((ret = fs_close()) < 0)
	{
		printf("FAILED fs_close failed, returned %d\n", ret);
		return 1;
	}
	// check file block
	fs_file file4;
	sd_read_block(block, 5);
	file4.fmt_iter = fmt_iter;
	file4.seq = 3;
	file4.len = 2;
	if (memcmp(&file4, block, sizeof(file3)) != 0)
	{
		printf("FAILED file block is incorrect\n");
		return 1;
	}
	// check first block
	memset(buf, 21, sizeof(buf));
	sd_read_block(block, 6);
	if (memcmp(buf, block, 500) != 0)
	{
		printf("FAILED file data is incorrect #1\n");
		return 1;
	}
	memset(buf, 22, sizeof(buf));
	if (memcmp(buf, block + 500, 12) != 0)
	{
		printf("FAILED file data is incorrect #2\n");
		return 1;
	}
	// check second block 
	memset(buf, 22, sizeof(buf));
	sd_read_block(block, 7);
	if (memcmp(buf, block, 2) != 0)
	{
		printf("FAILED file data is incorrect #3\n");
		return 1;
	}
	printf("PASSED\n");

	return 0;
} //}}}
