#ifndef SD_H
#define SD_H

#define SD_BLOCK_LEN 512
#define SD_MAX_RESP_TRIES 10
#define SD_MAX_RESET_TRIES 10
#define SD_INVALID_SECTOR 0xFFFFFFFF

struct sd_csd_t //{{{
{
	uint8_t ALWAYS_ONE:1;
	uint8_t CRC:7;
	uint8_t RESERVED1:2;
	uint8_t FILE_FORMAT:2;
	uint8_t TMP_WRITE_FORMAT:1;
	uint8_t PERM_WRITE_PROTECT:1;
	uint8_t COPY:1;
	uint8_t FILE_FORMAT_GRP:1;
	uint8_t RESERVED2:5;
	uint8_t WRITE_BL_PARTIAL:1;
	uint8_t WRITE_BL_LEN:4;
	uint8_t R2W_FACTOR:3;
	uint8_t RESERVED3:2;
	uint8_t WP_GRP_ENABLE:1;
	uint8_t WP_GRP_SIZE:7;
	uint8_t SECTOR_SIZE:7;
	uint8_t ERASE_BLK_EN:1;
	uint8_t RESERVED4:1;
	uint32_t C_SIZE:22;
	uint8_t RESERVED5:6;
	uint8_t DSR_IMP:1;
	uint8_t READ_BLK_MISALIGN:1;
	uint8_t WRITE_BLK_MISALIGN:1;
	uint8_t READ_BL_PARTIAL:1;
	uint8_t READ_BL_LEN:4;
	uint16_t CCC:12;
	uint8_t TRAN_SPEED:8;
	uint8_t NSAC:8;
	uint8_t TAAC:8;
	uint8_t RESERVED6:6;
	uint8_t CSD_STRUCTURE:2;
} __attribute__ ((__packed__)); 
typedef struct sd_csd_t sd_csd;
//}}}

typedef struct sd_info_t //{{{
{
	sd_csd csd;
} sd_info;
//}}}

int sd_init(sd_info* info);
void sd_command(uint8_t index, uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t crc, uint8_t* response, uint16_t response_len);
char sd_read_block(void* block, uint32_t block_num);
char sd_write_block(void* block, uint32_t block_num);
void sd_erase_blocks(uint32_t start_block_num, uint32_t end_block_num);

#endif
