#include <avr/io.h>
#include <string.h>
#include <stdio.h>

#include "../error.h"
#include "timer.h"
#include "spi.h"
#include "sd.h"
#include "gpio.h"
#include "led.h"


// Does not support MMC cards, only SD V1 and V2

static void swap_bytes(uint8_t* d, uint16_t len) //{{{
{
	uint8_t i_s = 0, i_e = len-1;
	uint8_t t;

	while (i_s < len && i_e >= 0 && i_s < i_e)
	{
		t = d[i_s];
		d[i_s] = d[i_e];
		d[i_e] = t;
		i_s++;
		i_e--;
	}
} //}}}

int sd_command(uint8_t index, uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t crc, uint8_t* response, uint16_t response_len) //{{{
{
	uint32_t tries;
	uint8_t command[6];
	uint8_t rx = 0;

	memset(response, 0, response_len);

	// fill command buffer
	command[0] = 0b01000000 | index; // command index
	command[1] = a1; // arg 0 
	command[2] = a2; // arg 1
	command[3] = a3; // arg 2
	command[4] = a4; // arg 3
	command[5] = crc; // CRC

	// transmit command
	spi_txrx(&SPIE, command, NULL, 6);

	// read until start bit
	tries = 0;
	response[0] = 0xFF;
	while ((response[0] & 0b10000000) != 0 && tries < SD_MAX_RESP_TRIES)
	{
		spi_txrx(&SPIE, NULL, response, 1);
		tries++;
	}

	if (tries >= SD_MAX_RESP_TRIES)
	{
		//led_on(LED_GREEN_bm);
		//while(1);
		return -1;
	}

	if (response_len > 1)
		spi_txrx(&SPIE, NULL, (response+1), response_len-1);

	/* read until the busy flag is cleared, 
	 * this also gives the SD card at least 8 clock pulses to give 
	 * it a chance to prepare for the next CMD */
	rx = 0;
	tries = 0;
	while (rx == 0 && tries < SD_MAX_BUSY_TRIES)
	{
		spi_txrx(&SPIE, NULL, &rx, 1);
		tries++;
	}

	if (tries >= SD_MAX_BUSY_TRIES)
		//led_on(LED_YELLOW_bm);
		//while(1);
		return -2;

	return 0;
} //}}}

int sd_init(sd_info* info) //{{{
{
	// init SPI
	PORTE.DIR = SPI_SS_PIN_bm | SPI_MOSI_PIN_bm | SPI_SCK_PIN_bm; // put the SPI pins in output mode
	gpio_on(&PORTE, SPI_SS_PIN_bm);
	SPIE.CTRL = SPI_ENABLE_bm | SPI_MASTER_bm | SPI_PRESCALER_DIV128_gc; 

	unsigned char resp[20];

	spi_txrx(&SPIE, NULL, NULL, 10); // send at least 74 clock pulses so the card enters native mode

	// keep trying to reset
	uint16_t tries = 0;
	int ret = 0;
	resp[0] = 0x00;
	while(resp[0] != 0x01 && tries < SD_MAX_RESET_TRIES)
	{
		gpio_off(&PORTE, SPI_SS_PIN_bm);
		ret = sd_command(0, 0x00, 0x00, 0x00, 0x00, 0x95, resp, 1); // CMD0
		gpio_on(&PORTE, SPI_SS_PIN_bm);
		tries++;
	}

	if (tries >= SD_MAX_RESET_TRIES || ret < 0)
		return -1;

	// check voltage range and check for V2
	gpio_off(&PORTE, SPI_SS_PIN_bm);
	sd_command(8, 0x00, 0x00, 0x01, 0xAA, 0x87, resp, 5); // CMD8, R7
	gpio_on(&PORTE, SPI_SS_PIN_bm);

	// V2 and voltage range is correct, have to do this for V2 cards
	if (resp[0] == 0x01)
	{
		if (!(resp[1] == 0 && resp[2] == 0 && resp[3] == 0x01 && resp[4] == 0xAA)) // voltage range is incorrect
			return -2;
	}

	// the initialization process
	while (resp[0] != 0x00) // 0 when the card is initialized
	{
		gpio_off(&PORTE, SPI_SS_PIN_bm);
		sd_command(55, 0x00, 0x00, 0x00, 0x00, 0x00, resp, 1); // CMD55
		gpio_on(&PORTE, SPI_SS_PIN_bm);
		if (resp[0] != 0x01)
			return -3;
		gpio_off(&PORTE, SPI_SS_PIN_bm);
		sd_command(41, 0x40, 0x00, 0x00, 0x00, 0x00, resp, 1); // ACMD41 with HCS (bit 30) HCS is ignored by V1 cards
		gpio_on(&PORTE, SPI_SS_PIN_bm);
	}

	// set the SPI clock to a much higher rate
	SPIE.CTRL &= ~SPI_PRESCALER_gm;
	SPIE.CTRL |= SPI_PRESCALER_DIV4_gc; 

	// get the CSR register 
	gpio_off(&PORTE, SPI_SS_PIN_bm);
	sd_command(9, 0x00, 0x00, 0x00, 0x00, 0x00, resp, 2+sizeof(info->csd)); // CMD9, R1 + DATA BYTE (0b11111110)
	gpio_on(&PORTE, SPI_SS_PIN_bm);
	swap_bytes(resp+2, sizeof(info->csd));
	memcpy(&(info->csd), resp+2, sizeof(info->csd));

	// check the OCR register to see if it's a high capacity card (V2)
	gpio_off(&PORTE, SPI_SS_PIN_bm);
	sd_command(58, 0x00, 0x00, 0x00, 0x00, 0x00, resp, 5); // CMD58
	gpio_on(&PORTE, SPI_SS_PIN_bm);
	if ((resp[1] & 0x40) == 0)
		return -4;

	return 0;
} //}}}

char sd_read_block(void* block, uint32_t block_num) //{{{
{
	// TODO bounds checking
	uint8_t rx = 0xFF;

	// send the single block command
	gpio_off(&PORTE, SPI_SS_PIN_bm);
	sd_command(17, (0xFF000000 & block_num) >> 24, (0xFF0000 & block_num) >> 16, (0xFF00 & block_num) >> 8, 0xFF & block_num, 0, &rx, 1); // CMD17

	// Could be an issue here where the last 8 of SD command contains the token, but I doubt this happens

	if (rx != 0x00)
		return 0;

	// read until the data token is received
	rx = 0xFF;
	while (rx != 0b11111110)
		spi_txrx(&SPIE, NULL, &rx, 1);

	spi_txrx(&SPIE, NULL, block, SD_BLOCK_LEN); // read the block
	spi_txrx(&SPIE, NULL, NULL, 2); // throw away the CRC
	spi_txrx(&SPIE, NULL, NULL, 1); // 8 cycles to prepare the card for the next command

	gpio_on(&PORTE, SPI_SS_PIN_bm);

	return 1;
} //}}}

char sd_write_block(void* block, uint32_t block_num) //{{{
{
	// TODO bounds checking
	uint8_t rx = 0xFF;
	uint8_t tx[2];

	// send the single block write 
	gpio_off(&PORTE, SPI_SS_PIN_bm);
	if (sd_command(24, (0xFF000000 & block_num) >> 24, (0xFF0000 & block_num) >> 16, (0xFF00 & block_num) >> 8, 0xFF & block_num, 0, &rx, 1) < 0) // CMD24
	{
		//led_on(LED_GREEN_bm);
		//while(1);
		halt(ERROR_SD_CMD24_FAILED);
	}

	// Could be an issue here where the last 8 of SD command contains the token, but I doubt this happens
	if (rx != 0x00)
	{
		//led_on(LED_YELLOW_bm);
		//while(1);
		halt(ERROR_SD_CMD24_ACK_FAILED);
	}

	// tick clock 8 times to start write operation 
	spi_txrx(&SPIE, NULL, NULL, 1);

	// write data token
	tx[0] = 0xFE;
	spi_txrx(&SPIE, tx, NULL, 1);

	// write data
	memset(tx, 0, sizeof(tx));
	spi_txrx(&SPIE, block, NULL, SD_BLOCK_LEN); // write the block
	spi_txrx(&SPIE, tx, NULL, 2); // write a blank CRC
	spi_txrx(&SPIE, NULL, &rx, 1); // get the response

	// check if the data is accepted
	if (!((rx & 0xE) >> 1 == 0x2))
	{
		//led_on(LED_RED_bm);
		//while(1);
		halt(ERROR_SD_CMD24_DATA_REJECTED);
	}

	// wait for the card to release the busy flag
	rx = 0;
	while (rx == 0)
		spi_txrx(&SPIE, NULL, &rx, 1);

	spi_txrx(&SPIE, NULL, NULL, 1); // 8 cycles to prepare the card for the next command

	gpio_on(&PORTE, SPI_SS_PIN_bm);
	return 0;
} //}}}
