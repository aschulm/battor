#include <avr/io.h>
#include <stdio.h>
#include <string.h>

#include "spi.h"
#include "gpio.h"
#include "dma.h"

#include "sram.h"

void sram_config_spi()
{
	PORTC.OUT |= USARTC1_TXD_PIN; // set the TXD pin high
	PORTC.OUT &= ~USARTC1_XCK_PIN; // set the XCK pin low
	PORTC.DIR |= USARTC1_TXD_PIN; // set the TXD pin to output
	PORTC.DIR |= USARTC1_XCK_PIN; // set the XCK pin to output
	PORTC.DIR &= ~USARTC1_RXD_PIN; // set the RX pin to input

	// set to the highest baud rate	
	USARTC1.BAUDCTRLA = 0;
	USARTC1.BAUDCTRLB = 0;

	PORTC.PIN5CTRL |= PORT_INVEN_bm; // invert the SCK pin for SPI mode 3

	// set transfer parameters
	USARTC1.CTRLC =
		USART_CMODE_MSPI_gc | 
		USART_UCPHA_bm; 

	USARTC1.CTRLA = 0; 
	USARTC1.CTRLB |= USART_RXEN_bm; // enable receiver
	USARTC1.CTRLB |= USART_TXEN_bm; // enable transmitter
}


void sram_unconfig_spi()
{
	PORTC.DIR &= ~USARTC1_TXD_PIN; // set the TXD pin to input
	PORTC.DIR &= ~USARTC1_XCK_PIN; // set the XCK pin to input
	PORTC.DIR &= ~USARTC1_RXD_PIN; // set the RX pin to input

	PORTC.PIN5CTRL &= ~PORT_INVEN_bm; // uninvert the SCK pin for SPI

	USARTC1.CTRLB &= ~USART_RXEN_bm; // disable receiver
	USARTC1.CTRLB &= ~USART_TXEN_bm; // disable transmitter
}

void sram_init()
{
	PORTE.PIN3CTRL |= PORT_OPC_PULLUP_gc; // pull up on CS Hold Pin
	PORTE.DIR |= SRAM_CS_PIN_gm;
	gpio_on(&PORTE, SRAM_CS_PIN_gm);
}

void usart_spi_txrx(const void* txd, void* rxd, uint16_t len)
{
	uint8_t* txd_b = (uint8_t*)txd;
	uint8_t* rxd_b = (uint8_t*)rxd;

 // clear out all rx data
 while ((USARTC1.STATUS & USART_RXCIF_bm) > 0)
		USARTC1.DATA;

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
			USARTC1.DATA;
	}
}

void* sram_write(void* addr, const void* src, size_t len)
{
	uint16_t addr_int = (uint16_t)addr;
	uint8_t hdr[] =
	{
		SRAM_CMD_WRITE,
#ifdef SRAM_24BIT_ADDR
		0,
#endif
		(addr_int >> 8) & 0xFF, addr_int & 0xFF, // sram expects big-endian
	};

	sram_config_spi();

	// begin transaction by setting CS and sendng write header
	gpio_off(&PORTE, SRAM_CS_PIN_gm);
	dma_sram_txrx(hdr, NULL, sizeof(hdr));
	//usart_spi_txrx(&hdr, NULL, sizeof(hdr));

	// send bytes
	dma_sram_txrx(src, NULL, len);
	//usart_spi_txrx(src, NULL, len);

	// unset CS to end transaction
	gpio_on(&PORTE, SRAM_CS_PIN_gm);

	sram_unconfig_spi();

	return addr;
}

void* sram_read(void* dst, const void* addr, size_t len)
{
	uint16_t addr_int = (uint16_t)addr;
	uint8_t hdr[] =
	{
		SRAM_CMD_READ,
#ifdef SRAM_24BIT_ADDR
		0,
#endif
		(addr_int >> 8) & 0xFF, addr_int & 0xFF, // sram expects big-endian
	};

	sram_config_spi();

	// begin transaction by setting CS and sendng read header
	gpio_off(&PORTE, SRAM_CS_PIN_gm);
	dma_sram_txrx(hdr, NULL, sizeof(hdr));

	// recv bytes
	dma_sram_txrx(NULL, dst, len);

	// unset CS to end transaction
	gpio_on(&PORTE, SRAM_CS_PIN_gm);

	sram_unconfig_spi();

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
			printf("FAILED (memcmp) wrote[0x%X] read[0x%X]\n",
				test_src[1],
				buf[0]);
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
