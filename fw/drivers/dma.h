#ifndef DMA_H
#define DMA_H

void dma_init(void* adcb_samples0, void* adcb_samples1, uint16_t samples_len);
void dma_start();
void dma_stop();

#endif
