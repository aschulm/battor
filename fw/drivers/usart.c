#include <avr/io.h>
#include <stdio.h>

#include "usart.h"

void usart_spi_txrx(USART_t* usart, const void* txd, void* rxd, uint16_t len)
{
	uint8_t* txd_b = (uint8_t*)txd;
	uint8_t* rxd_b = (uint8_t*)rxd;

 // clear out all rx data
 while ((usart->STATUS & USART_RXCIF_bm) > 0)
		usart->DATA;

	int i;
	for (i = 0; i < len; i++)
	{
		// transmit on the wire
		loop_until_bit_is_set(usart->STATUS, USART_DREIF_bp); // wait for tx buffer to empty
		if (txd_b != NULL)
			usart->DATA = txd_b[i];
		else
			usart->DATA = 0xFF;

		// read from the wire
		loop_until_bit_is_set(usart->STATUS, USART_RXCIF_bp); // wait for rx byte to be ready
		if (rxd_b != NULL)
			rxd_b[i] = usart->DATA;
		else
			usart->DATA;
	}
}

void usart_set_baud(USART_t* usart, uint16_t bsel, uint8_t bscale)
{
	usart->BAUDCTRLB = (bscale << USART_BSCALE_gp) | (bsel >> 8);
	usart->BAUDCTRLA = bsel & USART_BSEL_gm;
}
