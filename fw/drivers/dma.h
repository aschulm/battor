#ifndef DMA_H
#define DMA_H

void dma_init(int16_t* adca_samples0, int16_t* adca_samples1, int16_t* adcb_samples0, int16_t* adcb_samples1, uint16_t samples_len);
void dma_start();
void dma_stop();

#endif
