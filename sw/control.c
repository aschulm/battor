#include "common.h"

#include "uart.h"

#define CONTROL_WAIT_US 10000

void control(uint8_t type, uint16_t value1, uint16_t value2, uint8_t ack) //{{{
{
	int ret;
	control_message message;
	uint8_t type_rx = type+1;
	uint8_t uart_type = UART_TYPE_CONTROL_ACK+1;

	ret = -1;
	while (ret < 0 || uart_type != UART_TYPE_CONTROL_ACK || type_rx != type)
	{
		message.type = type;
		message.value1 = value1;
		message.value2 = value2;
		verb_printf("control: sending type:%d v1:%d v2:%d\n", type, value1, value2);
		uart_tx_bytes(UART_TYPE_CONTROL, &message, sizeof(message));

		if (ack)
		{
			ret = uart_rx_bytes(&uart_type, &type_rx, 1); // for flow control
			verb_printf("control: got ret:%d ack:%d\n", ret, type_rx);
		}
		else
		{
			return;
		}
	}
} //}}}
