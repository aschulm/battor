#ifndef ERROR_H
#define ERROR_H

#include "drivers/reset.h"
#include "interrupt.h"

typedef enum ERROR_enum
{
	ERROR_AMPPOT_SET_FAILED = 1,
	ERROR_FILPOT_SET_FAILED,
	ERROR_UART_OVERFLOW,
	ERROR_DMA_CH0_OVERFLOW,
	ERROR_DMA_CH1_OVERFLOW,
	ERROR_DMA_CH2_OVERFLOW,
	ERROR_DMA_CH3_OVERFLOW,
	ERROR_SD_CMD24_FAILED,
	ERROR_SD_CMD24_ACK_FAILED,
	ERROR_SD_CMD24_DATA_REJECTED,
	ERROR_SD_WRITE_IN_PROGRESS,
	ERROR_RINGBUF_WRITE_FAIL,
	ERROR_SAMPLERATE_SET_WRONG_MODE,
	ERROR_FS_OPEN_FAIL,
	ERROR_FS_WRITE_FAIL,
	ERROR_FS_CLOSE_FAIL,
	ERROR_FS_FORMAT_FAIL,
	ERROR_SD_CMD25_FAILED,
	ERROR_SD_CMD25_ACK_FAILED,
	ERROR_SD_CMD25_DATA_REJECTED,
	ERROR_SD_MULTI_BLOCK_OUT_OF_ORDER,
	ERROR_SD_INIT_FAIL,
} ERROR_t;

extern uint8_t g_error_last;

void dump_stack();
void print_buffer(uint8_t* buf, uint16_t len);

void inline halt(uint8_t code) //{{{
{
	interrupt_disable();

	// remember last error
	g_error_last = code;

	printf("HALT %u\n", code);

	dump_stack();

	reset();
} //}}}

extern uint8_t g_error_last;

#endif
