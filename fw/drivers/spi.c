#include <avr/io.h>
#include <stdio.h>

#include "spi.h"

void spi_txrx(SPI_t* spi, const void* txd, void* rxd, uint16_t len)
{
	uint8_t* txd_b = (uint8_t*)txd;
	uint8_t* rxd_b = (uint8_t*)rxd;
	// write and read
	int i;
	for (i = 0; i < len; i++)
	{ 
		// write to the wire
		if (txd_b != NULL)
			spi->DATA = txd_b[i];
		else 
			spi->DATA = 0xFF; // needed for things like SD that have long only RX

		loop_until_bit_is_set(spi->STATUS, SPI_IF_bp); // wait until the write is over
 
		// read from the wire
		if (rxd_b != NULL)
			rxd_b[i] = spi->DATA;
		else
			spi->DATA; // don't care about the RX
	}
}
