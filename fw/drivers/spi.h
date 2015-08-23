#ifndef SPI_H
#define SPI_H

#define SPI_SS_PIN_bm (1<<4)
#define SPI_MOSI_PIN_bm (1<<5)
#define SPI_MISO_PIN_bm (1<<6)
#define SPI_SCK_PIN_bm (1<<7)

#define SPI_MISO_PIN_bp 6

void spi_txrx(SPI_t* spi, const void* txd, void* rxd, uint16_t len);

#endif
