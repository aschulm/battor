#include "common.h"

#include "uart.h"

int control(uint8_t type, uint16_t value1, uint16_t value2, uint8_t wait_for_ack) //{{{
{
	int ret;
	control_message message;
	uint8_t type_rx = type+1;
	uint8_t uart_type = UART_TYPE_CONTROL_ACK+1;
	control_ack ack;

	memset(&ack, 0, sizeof(ack));

	ret = -1;
	while (ret < 0 || uart_type != UART_TYPE_CONTROL_ACK || ack.type != type)
	{
		message.type = type;
		message.value1 = value1;
		message.value2 = value2;
		verb_printf("control: sending type:%d v1:%d v2:%d\n", type, value1, value2);
		if (uart_tx_bytes(UART_TYPE_CONTROL, &message, sizeof(message)) < 0)
		{
			fprintf(stderr, "ERROR: failed to send control message\n");
			exit(EXIT_FAILURE);
		}

		if (wait_for_ack)
		{
			usleep(CONTROL_SLEEP_US);
			ret = uart_rx_bytes(&uart_type, &ack, sizeof(ack));
			verb_printf("control: got ack ret:%d type:%d value:%d\n", ret, ack.type, ack.value);
		}
		else
		{
			return -1;
		}
	}

	return ack.value;
} //}}}
