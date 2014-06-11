#ifndef DMA_H
#define DMA_H

void dma_init(uint8_t* adca_samples0, uint8_t* adca_samples1, uint8_t* adcb_samples0, uint8_t* adcb_samples1, uint16_t samples_len);
void dma_start();
void dma_stop();

#endif
