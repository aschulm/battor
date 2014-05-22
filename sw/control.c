#include "common.h"

#include "uart.h"

void control(uint8_t type, uint16_t value1, uint16_t value2) //{{{
{
	uint8_t c;
	control_message message;

	message.delim[0] = 0xBA;
	message.delim[1] = 0x77;
	message.type = type;
	message.value1 = value1;
	message.value2 = value2;
	uart_write(&message, sizeof(message));
	c = 0;
	while(c != 0x01)
		uart_read(&c, 1);
} //}}}
