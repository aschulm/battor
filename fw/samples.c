#include "common.h"

#include "error.h"
#include "samples.h"

uint8_t g_samples_uart_seqnum;

void samples_uart_write(uint8_t* v_s, uint8_t* i_s) //{{{
{
	uint16_t len = SAMPLES_LEN;

	// write frame delimiter
	uart_tx_byte(SAMPLES_DELIM0);
	uart_tx_byte(SAMPLES_DELIM1);

	// write seqnum
	uart_tx_byte(g_samples_uart_seqnum);

	// write number of samples
	uart_tx_bytes(&len, sizeof(len)); 

	// write current samples
	uart_tx_bytes(v_s, len);
	// write voltage samples
	uart_tx_bytes(i_s, len);

	g_samples_uart_seqnum++;
} //}}}

/*void samples_sd_write(uint8_t* v_s, uint8_t* i_s, FIL* file) //{{{
{
	uint16_t len = SAMPLES_LEN, write_len;

	if (f_write(file, &len, sizeof(len), &write_len) != FR_OK) // write number of samples
		halt();
	if (write_len != sizeof(len)) 
		halt();
	if (f_write(file, v_s, len, &write_len) != FR_OK) // write current samples
		halt();
	if (write_len != len) 
		halt();
	if (f_write(file, i_s, len, &write_len) != FR_OK) // write voltage samples
		halt();
	if (write_len != len) 
		halt();
	if (f_sync(file) != FR_OK)
		halt();
} //}}} */
