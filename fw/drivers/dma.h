#ifndef DMA_H
#define DMA_H

extern DMA_CH_t* g_uart_ch;
extern volatile uint8_t g_uart_tx_paused;

void dma_init(uint8_t spi_uart_only);
void dma_start(void* adcb_samples0, void* adcb_samples1);
void dma_stop();
uint32_t dma_get_sample_count();
void dma_uart_tx(const void* data, uint16_t len);
uint8_t dma_uart_tx_ready();
void dma_spi_txrx(USART_t* usart, const void* txd, void* rxd, uint16_t len);

inline void dma_uart_tx_pause(uint8_t on_off) //{{{
{
	g_uart_tx_paused = on_off;

	if (!(g_uart_ch->TRIGSRC == DMA_CH_TRIGSRC_OFF_gc ||
		g_uart_ch->TRIGSRC == DMA_CH_TRIGSRC_USARTD0_DRE_gc))
		return;

	if (on_off)
		g_uart_ch->TRIGSRC = DMA_CH_TRIGSRC_OFF_gc;
	else
		g_uart_ch->TRIGSRC = DMA_CH_TRIGSRC_USARTD0_DRE_gc;
} //}}}

#endif
