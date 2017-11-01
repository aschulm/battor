#include "common.h"

#include "control.h"
#include "blink.h"
#include "drivers.h"

uint8_t g_error_last __attribute__ ((section (".noinit"))); // do not zero out on reset

void dump_stack() //{{{
{
	uint8_t* sp = (uint8_t*)(SP + 1);
	uint16_t i;

	printf("Stack dump notes:                                              \n\
		1. 3 bytes for each return address                                   \n\
		2. return addresses must be multiplied by 2 to compare with disasm   \n\
	");

	for (i = 0; i < 50; i++)
	{
		printf("%02X ", sp[i]);
	}
	printf("\n");
} //}}}

void print_buffer(uint8_t* buf, uint16_t len) //{{{
{
	uint16_t i;
	printf("[");
	for (i = 0; i < len; i++)
		printf(" %02X", buf[i]);
	printf(" ]");
} //}}}
