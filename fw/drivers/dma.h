#ifndef DMA_H
#define DMA_H

void dma_init(uint8_t spi_uart_only);
void dma_start(void* adcb_samples0, void* adcb_samples1);
void dma_stop();
uint32_t dma_get_sample_count();

void dma_uart_tx(const void* data, uint16_t len);
uint8_t dma_uart_tx_ready();
inline void dma_uart_tx_pause(uint8_t on_off);
void dma_spi_txrx(USART_t* usart, const void* txd, void* rxd, uint16_t len);

#endif
