#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

#include "button.h"

void button_init()
{
	PORTE.DIR &= BUTTON_bm;
	// button requires a pullup
	PORTE.PIN0CTRL |= PORT_OPC_PULLUP_gc;
}

int button_self_test()
{
	printf("button self test\n");
	printf("waiting for button press...");
	while ((PORTE.IN & BUTTON_bm) > 0);
	printf("PASSED\n");
	return 0;
}
