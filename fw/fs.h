#ifndef FS_H
#define FS_H

#define FS_SUPERBLOCK_IDX 0
#define FS_VERSION 0

typedef struct fs_superblock_
{
	uint8_t magic[8];
	uint8_t ver;
	uint32_t fmt_iter;
} fs_superblock;

typedef struct fs_file_
{
	uint32_t seq;
	uint32_t len;
	uint32_t fmt_iter;
} fs_file;

typedef enum FS_ERROR_enum
{
	FS_ERROR_NONE = 0,
	FS_ERROR_NO_EXISTING_FILE = -1,
	FS_ERROR_FILE_TOO_LONG = -2,
	FS_ERROR_FILE_NOT_OPEN = -3,
	FS_ERROR_SD_READ = -4,
	FS_ERROR_SD_WRITE = -5,
	FS_ERROR_WRITE_IN_PROGRESS = -6,
} FS_ERROR_t;

int fs_open(uint8_t new_file);
int fs_close();
int fs_write(void* buf, uint16_t len);
void fs_read();
void fs_update();
int fs_busy();
int fs_self_test();

#endif
