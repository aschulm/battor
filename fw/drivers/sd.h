#ifndef SD_H
#define SD_H

#define SD_BLOCK_LEN 512
#define SD_MAX_RESP_TRIES 10
#define SD_MAX_RESET_TRIES 10
#define SD_MAX_BUSY_TRIES 1000
#define SD_INVALID_SECTOR 0xFFFFFFFF

int sd_init();
int sd_command(uint8_t index, uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t crc, uint8_t* response, uint16_t response_len);
int sd_read_block(void* block, uint32_t block_num);
uint32_t sd_capacity(); 
int sd_write_block_start(void* block, uint32_t block_num);
int sd_write_block_update();
void sd_erase_blocks(uint32_t start_block_num, uint32_t end_block_num);
int sd_self_test();
void sd_overrun();

#endif
