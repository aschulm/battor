#ifndef SD_H
#define SD_H

#define SD_BLOCK_LEN 512
#define SD_MAX_RESP_TRIES 10
#define SD_MAX_RESET_TRIES 10
#define SD_MAX_BUSY_TRIES 1000
#define SD_INVALID_SECTOR 0xFFFFFFFF

struct sd_csd_t //{{{
{
	uint8_t always_one:1;
	uint8_t crc:7;
	uint8_t reserved1:2;
	uint8_t file_format:2;
	uint8_t tmp_write_format:1;
	uint8_t perm_write_protect:1;
	uint8_t copy:1;
	uint8_t file_format_grp:1;
	uint8_t reserved2:5;
	uint8_t write_bl_partial:1;
	uint8_t write_bl_len:4;
	uint8_t r2w_factor:3;
	uint8_t reserved3:2;
	uint8_t wp_grp_enable:1;
	uint8_t wp_grp_size:7;
	uint8_t sector_size:7;
	uint8_t erase_blk_en:1;
	uint8_t reserved4:1;
	uint32_t c_size:22;
	uint8_t reserved5:6;
	uint8_t dsr_imp:1;
	uint8_t read_blk_misalign:1;
	uint8_t WRITE_BLK_MISALIGN:1;
	uint8_t read_bl_partial:1;
	uint8_t read_bl_len:4;
	uint16_t CCC:12;
	uint8_t tran_speed:8;
	uint8_t nsac:8;
	uint8_t taac:8;
	uint8_t reserved6:6;
	uint8_t csd_structure:2;
} __attribute__ ((__packed__)); 
typedef struct sd_csd_t sd_csd;
//}}}

int sd_init();
int sd_command(uint8_t index, uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t crc, uint8_t* response, uint16_t response_len);
int sd_read_block(void* block, uint32_t block_num);
uint32_t sd_capacity(); 
int sd_write_block_start(void* block, uint32_t block_num);
int sd_write_block_update();
void sd_erase_blocks(uint32_t start_block_num, uint32_t end_block_num);
int sd_self_test();

#endif
