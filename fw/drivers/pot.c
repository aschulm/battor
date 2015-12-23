#include <avr/io.h>
#include <stdio.h>

#include "gpio.h"
#include "spi.h"
#include "pot.h"
#include "timer.h"

static inline uint16_t byte_swap16(uint16_t b)
{
	return ((b & 0xFF) << 8) | ((b & 0xFF00) >> 8);
}

static void pot_config_spi()
{
	// swap SCK and MOSI for USART peripheral compatibility
	PORTC.REMAP |= PORT_SPI_bm; 

	PORTC.PIN6CTRL |= PORT_OPC_PULLUP_gc; // pot requires a pullup on the SDO (MISO) pin
	SPIC.CTRL = SPI_ENABLE_bm | SPI_MASTER_bm | SPI_MODE_1_gc | SPI_PRESCALER_DIV128_gc; 
	PORTC.OUT &= ~(SPI_MOSI_PIN_bm | SPI_SCK_PIN_bm); // output pins should be low
	PORTC.DIR |= SPI_MOSI_PIN_bm | SPI_SCK_PIN_bm; // enable output pins
}

static void pot_unconfig_spi()
{
	// swap SCK and MOSI for USART peripheral compatibility
	PORTC.REMAP &= ~PORT_SPI_bm; 

	PORTC.PIN6CTRL = 0; // pot required a pullup on the SDO (MISO) pin, disable it
	SPIC.CTRL = 0; // disable the SPI peripheral
	PORTC.DIR &= ~(SPI_MOSI_PIN_bm | SPI_SCK_PIN_bm); // unconfigure output pins
}

// the pots are not normal SPI devices, you have to put the output pin in high impedience mode when done
// else two pots on the same MISO can not output
static void pot_high_impedience_sdo(uint8_t pot_cs_pin)
{
	uint8_t tx[2], rx[2];

	pot_config_spi();

	tx[0]=0x80;
	tx[1]=0x01;
	gpio_off(&PORTC, pot_cs_pin);
	spi_txrx(&SPIC, tx, rx, 2);
	gpio_on(&PORTC, pot_cs_pin);

	tx[0] = 0x00;
	tx[1] = 0x00;
	gpio_off(&PORTC, pot_cs_pin);
	spi_txrx(&SPIC, tx, rx, 2);
	gpio_on(&PORTC, pot_cs_pin);

	pot_unconfig_spi();
}

static uint16_t pot_send_command(uint8_t pot_cs_pin, uint8_t command, uint16_t data)
{
	uint8_t tx[2], rx[2];

	pot_config_spi();

	tx[0] = (command << 2) | ((data & 0x300) >> 8);
	tx[1] = (data & 0xFF);
	gpio_off(&PORTC, pot_cs_pin);
	spi_txrx(&SPIC, tx, rx, 2);
	gpio_on(&PORTC, pot_cs_pin);
	timer_sleep_ms(1);

	pot_unconfig_spi();

	return ((rx[0] & 0x3) << 8) | (rx[1] & 0xFF);
}

uint16_t pot_wiperpos_get(uint8_t pot_cs_pin)
{
	uint16_t pos = 0;
	pot_send_command(pot_cs_pin, 0x2, 0); // read RDAC
	pos = pot_send_command(pot_cs_pin, 0x0, 0); // NOP to get data
	pot_high_impedience_sdo(pot_cs_pin);
	return pos;
}

void pot_wiperpos_set(uint8_t pot_cs_pin, uint16_t pos)
{
	pot_send_command(pot_cs_pin, 0x1, pos); // write RDAC
	pot_high_impedience_sdo(pot_cs_pin);
}

void pot_init()
{
	gpio_on(&PORTC, POT_AMP_CS_PIN_gm);
	gpio_on(&PORTC, POT_FIL_CS_PIN_gm);
	PORTC.DIR |= POT_AMP_CS_PIN_gm | POT_FIL_CS_PIN_gm; // chip select pins in output mode

	pot_config_spi();

	// start with both SDOs in high-impedience mode
	pot_high_impedience_sdo(POT_AMP_CS_PIN_gm);
	pot_high_impedience_sdo(POT_FIL_CS_PIN_gm);

	// software reset and unprotect the RDAC register
	pot_send_command(POT_AMP_CS_PIN_gm, 0x4, 0);
	pot_send_command(POT_AMP_CS_PIN_gm, 0x7, 0x2); 
	pot_high_impedience_sdo(POT_AMP_CS_PIN_gm);
	pot_send_command(POT_FIL_CS_PIN_gm, 0x4, 0);
	pot_send_command(POT_FIL_CS_PIN_gm, 0x7, 0x2);
	pot_high_impedience_sdo(POT_FIL_CS_PIN_gm);

	pot_unconfig_spi();
}

int pot_self_test()
{
	uint16_t pos = 0;
	printf("potentiometer self test\n");

	printf("amppot set and get...");
	pot_wiperpos_set(POT_AMP_CS_PIN_gm, 15);
	pos = pot_wiperpos_get(POT_AMP_CS_PIN_gm);
	if (pos != 15)
	{
		printf("FAILED wrote[15] read[%d]\n",
			pos);
		return 1;
	}
	printf("PASSED\n");

	printf("filpot set and get...");
	pot_wiperpos_set(POT_FIL_CS_PIN_gm, 1000);
	pos = pot_wiperpos_get(POT_FIL_CS_PIN_gm);
	if (pos != 1000)
	{
		printf("FAILED wrote[20] read[%d]\n",
			pos);
		return 1;
	}
	printf("PASSED\n");

	return 0;
}
