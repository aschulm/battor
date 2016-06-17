#include <avr/io.h>
#include <stdio.h>

#include "gpio.h"
#include "spi.h"
#include "timer.h"
#include "usart.h"

#include "pot.h"

static inline uint16_t byte_swap16(uint16_t b)
{
	return ((b & 0xFF) << 8) | ((b & 0xFF00) >> 8);
}

static void pot_config_spi()
{
	PORTC.OUT |= USART1_TXD_PIN; // set the TXD pin high
	PORTC.OUT &= ~USART1_XCK_PIN; // set the XCK pin low
	PORTC.DIR |= USART1_TXD_PIN; // set the TXD pin to output
	PORTC.DIR |= USART1_XCK_PIN; // set the XCK pin to output
	PORTC.DIR &= ~USART1_RXD_PIN; // set the RX pin to input

	// set to a low baud rate that works with the pull-up on MISO
	usart_set_baud(&USARTC1, USART_BSEL_SPI_20000BPS, USART_BSCALE_SPI_20000BPS);

	// set transfer parameters
	USARTC1.CTRLC =
		USART_CMODE_MSPI_gc | 
		USART_UCPHA_bm; 

	USARTC1.CTRLA = 0; 
	USARTC1.CTRLB |= USART_RXEN_bm; // enable receiver
	USARTC1.CTRLB |= USART_TXEN_bm; // enable transmitter

	PORTC.PIN6CTRL |= PORT_OPC_PULLUP_gc; // pot requires a pullup on the SDO (MISO) pin

	// Wait for the transmitter to go through the first clock cycle.
	// this is needed because the clock rate is so slow here that the transmitter
	// may not be ready for the first clock cycle
	timer_sleep_ms(2);
}

static void pot_unconfig_spi()
{
	PORTC.OUT &= ~USART1_TXD_PIN; // set the TXD pin low 
	PORTC.DIR &= ~USART1_TXD_PIN; // set the TXD pin to input
	PORTC.DIR &= ~USART1_XCK_PIN; // set the XCK pin to input
	PORTC.DIR &= ~USART1_RXD_PIN; // set the RX pin to input

	PORTC.PIN6CTRL = 0; // pot required a pullup on the SDO (MISO) pin, disable it
	USARTC1.CTRLB &= ~USART_RXEN_bm; // disable receiver
	USARTC1.CTRLB &= ~USART_TXEN_bm; // disable transmitter

	// Wait for the transmitter to go through the last clock cycle.
	// this is needed because the clock rate is so slow here that the transmitter
	// may be going through a clock cycle as the next transmission begins 
	timer_sleep_ms(2);
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
	usart_spi_txrx(&USARTC1, tx, rx, 2);
	gpio_on(&PORTC, pot_cs_pin);

	tx[0] = 0x00;
	tx[1] = 0x00;
	gpio_off(&PORTC, pot_cs_pin);
	usart_spi_txrx(&USARTC1, tx, rx, 2);
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
	usart_spi_txrx(&USARTC1, tx, rx, 2);
	gpio_on(&PORTC, pot_cs_pin);

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
	timer_sleep_ms(5); // need to wait at least 0.6ms for reset
	pot_send_command(POT_AMP_CS_PIN_gm, 0x7, 0x2); 
	pot_high_impedience_sdo(POT_AMP_CS_PIN_gm);
	pot_send_command(POT_FIL_CS_PIN_gm, 0x4, 0);
	timer_sleep_ms(5); // need to wait at least 0.6ms for reset
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
