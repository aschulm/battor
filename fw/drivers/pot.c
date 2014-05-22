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

// the pots are not normal SPI devices, you have to put the output pin in high impedience mode when done
// else two pots on the same MISO can not output
static void pot_high_impedience_sdo(uint8_t pot_cs_pin)
{
	uint16_t tx, rx;
	tx = byte_swap16(0x8001);
	gpio_off(&PORTC, pot_cs_pin);
	spi_txrx(&SPIC, &tx, &rx, 2);
	gpio_on(&PORTC, pot_cs_pin);

	tx = 0x0000;
	gpio_off(&PORTC, pot_cs_pin);
	spi_txrx(&SPIC, &tx, &rx, 2);
	gpio_on(&PORTC, pot_cs_pin);
}

static uint16_t pot_send_command(uint8_t pot_cs_pin, uint16_t command, uint16_t data)
{
	uint16_t tx, rx;
	tx = byte_swap16((command << 10) | data);
	gpio_off(&PORTC, pot_cs_pin);
	spi_txrx(&SPIC, &tx, &rx, 2);
	gpio_on(&PORTC, pot_cs_pin);
	timer_sleep_ms(10);
	return byte_swap16(rx) & 0x3FF;
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
	PORTC.PIN6CTRL |= PORT_OPC_PULLUP_gc; // pot requires a pullup on the SDO (MISO) pin
	gpio_on(&PORTC, POT_AMP_CS_PIN_gm);
	gpio_on(&PORTC, POT_FIL_CS_PIN_gm);
	PORTC.DIR |= SPI_SS_PIN_bm | SPI_MOSI_PIN_bm | SPI_SCK_PIN_bm | POT_AMP_CS_PIN_gm | POT_FIL_CS_PIN_gm; // put the SPI pins in output mode

	SPIC.CTRL = SPI_ENABLE_bm | SPI_MASTER_bm | SPI_MODE_1_gc | SPI_PRESCALER_DIV128_gc; 

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
}
