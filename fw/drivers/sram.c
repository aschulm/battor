#include <avr/io.h>
#include <stdio.h>
#include <string.h>

#include "spi.h"
#include "gpio.h"

#include "sram.h"

void sram_init()
{
	PORTC.OUT |= USARTC1_TXD_PIN; // set the TXD pin high
	PORTC.DIR |= USARTC1_TXD_PIN; // set the TXD pin to output
	PORTC.DIR &= ~USARTC1_RXD_PIN; // set the RX pin to input

	// set baud rate with BSEL and BSCALE values
	USARTC1.BAUDCTRLB = USART_BSCALE_2000000BPS << USART_BSCALE_gp;
	USARTC1.BAUDCTRLA = USART_BSEL_2000000BPS & USART_BSEL_gm;

	PORTC.PIN5CTRL |= PORT_INVEN_gc; // invert the SCK pin for SPI mode 3

	// set transfer parameters
	USARTC1.CTRLC =
		USART_CMODE_MSPI_gc |
		USART_PMODE_DISABLED_gc |
		USART_CHSIZE_8BIT_gc |
		USART_UCPHA_gc; 

	USARTC1.CTRLA = USART_RXCINTLVL_HI_gc; // interrupt for receive 
	USARTC1.CTRLB |= USART_RXEN_bm; // enable receiver
	USARTC1.CTRLB |= USART_TXEN_bm; // enable transmitter

	PORTE.PIN3CTRL |= PORT_OPC_PULLUP_gc; // pull up on CS Hold Pin
	PORTE.DIR |= SRAM_CS_PIN_gm;
	gpio_on(&PORTE, SRAM_CS_PIN_gm);
}

void usart-spi_txrx(const void* txd, void* rxd, uint16_t len)
{
	uint8_t rx_null = 0;
	uint8_t* txd_b = (uint8_t*)txd;
	uint8_t* rxd_b = (uint8_t*)rxd;

	int i;
	for (i = 0; i < len; i++)
	{
		// transmit on the wire
		loop_until_bit_is_set(USARTC1.STATUS, USART_DREIF_bp); // wait for tx buffer to empty
		if (txd_b != NULL)
			USARTC1.DATA = txd_b[i];
		else
			USARTC1.DATA = 0xFF;

		// read from the wire
		loop_until_bit_is_set(USARTC1.STATUS, USART_RXCIF_bp); // wait for rx byte to be ready
		if (rxd_b != NULL)
			rxd_b[i] = USARTC1.DATA;
		else
			rx_null = USARTC1.DATA;
	}
}

void* sram_write(void* addr, const void* src, size_t len)
{
	uint16_t addr_int = (uint16_t)addr;
	uint8_t hdr[] =
	{
		SRAM_CMD_WRITE,
		(addr_int >> 8) & 0xFF, addr_int & 0xFF // sram expects big-endian
	};

	//sram_config_spi();
	// begin transaction by setting CS and sendng write header
	gpio_off(&PORTE, SRAM_CS_PIN_gm);
	usart-spi_txrx(&SPIC, &hdr, NULL, sizeof(hdr));

	// send bytes
	usart-spi_txrx(&SPIC, src, NULL, len);

	// unset CS to end transaction
	gpio_on(&PORTE, SRAM_CS_PIN_gm);

	return addr;
}

void* sram_read(void* dst, const void* addr, size_t len)
{
	uint16_t addr_int = (uint16_t)addr;
	uint8_t hdr[] =
	{
		SRAM_CMD_READ,
		(addr_int >> 8) & 0xFF, addr_int & 0xFF // sram expects big-endian
	};

	//sram_config_spi();

	// begin transaction by setting CS and sendng read header
	gpio_off(&PORTE, SRAM_CS_PIN_gm);
 	usart-spi_txrx(&SPIC, &hdr, NULL, sizeof(hdr));

	// recv bytes
 	usart-spi_txrx(&SPIC, NULL, dst, len);

	// unset CS to end transaction
	gpio_on(&PORTE, SRAM_CS_PIN_gm);

	return dst;
}

int sram_self_test() {
		uint8_t test_src[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99};
		uint8_t buf[10];

		printf("sram self test\n");

		printf("sram write 1...");
		memset(buf, 0, sizeof(buf));
		sram_write((void*)0x0000, test_src + 1, 1); // +1 to get non-zero src
		sram_read(buf, 0x0000, 1);
		if (memcmp(test_src + 1, buf, 1) != 0)
		{
			printf("FAILED (memcmp)\n");
			return 1;
		}
		printf("PASSED\n");

		printf("sram write 10...");
		memset(buf, 0, sizeof(buf));
		sram_write((void*)0x0000, test_src, 10);
		sram_read(buf, 0x0000, 10);
		if (memcmp(test_src, buf, 10) != 0)
		{
			printf("FAILED (memcmp)\n");
			return 1;
		}
		printf("PASSED\n");

		printf("sram write 10 high address...");
		memset(buf, 0, sizeof(buf));
		sram_write((void*)63000, test_src, 10);
		sram_read(buf, (void*)63000, 10);
		if (memcmp(test_src, buf, 10) != 0)
		{
			printf("FAILED (memcmp)\n");
			return 1;
		}
		printf("PASSED\n");

		return 0;
}
