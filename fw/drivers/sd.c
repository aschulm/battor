#include <avr/io.h>
#include <string.h>
#include <stdio.h>

#include "../error.h"
#include "timer.h"
#include "spi.h"
#include "gpio.h"
#include "led.h"
#include "dma.h"
#include "usart.h"

#include "sd.h"

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
	usart_spi_txrx(&USARTE1, command, NULL, 6);

	// read until start bit
	tries = 0;
	response[0] = 0xFF;
	while ((response[0] & 0b10000000) != 0 && tries < SD_MAX_RESP_TRIES)
	{
		usart_spi_txrx(&USARTE1, NULL, response, 1);
		tries++;
	}

	if (tries >= SD_MAX_RESP_TRIES)
	{
		//led_on(LED_GREEN_bm);
		//while(1);
		return -1;
	}

	if (response_len > 1)
		usart_spi_txrx(&USARTE1, NULL, (response+1), response_len-1);

	/* read until the busy flag is cleared, 
	 * this also gives the SD card at least 8 clock pulses to give 
	 * it a chance to prepare for the next CMD */
	rx = 0;
	tries = 0;
	while (rx == 0 && tries < SD_MAX_BUSY_TRIES)
	{
		usart_spi_txrx(&USARTE1, NULL, &rx, 1);
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
	PORTE.OUT |= USART1_TXD_PIN; // set the TXD pin high
	PORTE.OUT &= ~USART1_XCK_PIN; // set the XCK pin low
	PORTE.DIR |= USART1_TXD_PIN; // set the TXD pin to output
	PORTE.DIR |= USART1_XCK_PIN; // set the XCK pin to output
	PORTE.DIR &= ~USART1_RXD_PIN; // set the RX pin to input

	PORTE.DIR |= SPI_SS_PIN_bm;
	gpio_on(&PORTE, SPI_SS_PIN_bm);

	// set to a low baud rate for initial SD CARD 
	USARTE1.BAUDCTRLB = USART_BSCALE_100000BPS << USART_BSCALE_gp;
	USARTE1.BAUDCTRLA = USART_BSEL_100000BPS & USART_BSEL_gm;

	// set transfer parameters
	USARTE1.CTRLC = USART_CMODE_MSPI_gc;

	USARTE1.CTRLA = 0; 
	USARTE1.CTRLB |= USART_RXEN_bm; // enable receiver
	USARTE1.CTRLB |= USART_TXEN_bm; // enable transmitter

	unsigned char resp[20];

	usart_spi_txrx(&USARTE1, NULL, NULL, 10); // send at least 74 clock pulses so the card enters native mode

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
	USARTE1.BAUDCTRLA = 0;
	USARTE1.BAUDCTRLB = 0;

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

int sd_read_block(void* block, uint32_t block_num) //{{{
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
		usart_spi_txrx(&USARTE1, NULL, &rx, 1);

	usart_spi_txrx(&USARTE1, NULL, block, SD_BLOCK_LEN); // read the block
	usart_spi_txrx(&USARTE1, NULL, NULL, 2); // throw away the CRC
	usart_spi_txrx(&USARTE1, NULL, NULL, 1); // 8 cycles to prepare the card for the next command

	gpio_on(&PORTE, SPI_SS_PIN_bm);

	return 1;
} //}}}

int sd_write_block_start(void* block, uint32_t block_num) //{{{
{
	// TODO bounds checking
	uint8_t rx = 0xFF;
	uint8_t tx[2];

	// send the single block write 
	gpio_off(&PORTE, SPI_SS_PIN_bm);
	if (sd_command(24, (0xFF000000 & block_num) >> 24, (0xFF0000 & block_num) >> 16, (0xFF00 & block_num) >> 8, 0xFF & block_num, 0, &rx, 1) < 0) // CMD24
	{
		halt(ERROR_SD_CMD24_FAILED);
	}

	// Could be an issue here where the last 8 of SD command
	// contains the token, but I doubt this happens
	if (rx != 0x00)
	{
		halt(ERROR_SD_CMD24_ACK_FAILED);
	}

	// tick clock 8 times to start write operation 
	usart_spi_txrx(&USARTE1, NULL, NULL, 1);

	// write data token
	tx[0] = 0xFE;
	usart_spi_txrx(&USARTE1, tx, NULL, 1);

	// write data
	memset(tx, 0, sizeof(tx));
	dma_spi_txrx(&USARTE1, block, NULL, SD_BLOCK_LEN); // write the block
	usart_spi_txrx(&USARTE1, tx, NULL, 2); // write a blank CRC
	usart_spi_txrx(&USARTE1, NULL, &rx, 1); // get the response

	// check if the data is accepted
	if (!((rx & 0xE) >> 1 == 0x2))
	{
		halt(ERROR_SD_CMD24_DATA_REJECTED);
	}

	/*
	 * NOTE: This function must follow with calls to
	 * sd_write_block_update() until the write completes.
	 */

	return 1;
} //}}}

int sd_write_block_update() //{{{
{
	int i;
	uint8_t rx[32];

	memset(rx, 0, sizeof(rx));

	// cycle the clock several times to advance write and get response for each
	dma_spi_txrx(&USARTE1, NULL, &rx, sizeof(rx));

	for (i = 0; i < sizeof(rx); i++)
	{
		// is transfer complete?
		if (rx[i] != 0)
		{
			/*
			 * Transfer is complete.
			 * give the SD card 8 clock cycles to prepare the card for the next command.
			 */
			usart_spi_txrx(&USARTE1, NULL, NULL, 1);
			gpio_on(&PORTE, SPI_SS_PIN_bm);
			return 0;
		}
	}

	// transfer is still in progress...
	return -1;
} //}}}

int sd_self_test() //{{{
{
	int i = 0;
	uint8_t block[SD_BLOCK_LEN];

	printf("sd self test\n");

	printf("one block write and read...");
	memset(block, 16, sizeof(block));
	sd_write_block_start(block, 10);
	while(sd_write_block_update() < 0);
	memset(block, 0, sizeof(block));
	sd_read_block(block, 10);
	for (i = 0; i < SD_BLOCK_LEN; i++)
	{
		if (block[i] != 16)
		{
			printf("FAILED wrote[16] read[%d]\n",
					block[i]);
			return -1;
		}
	}
	printf("PASSED\n");

	return 0;
} //}}}
