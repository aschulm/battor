#include "common.h"

#include "control.h"
#include "blink.h"
#include "interrupt.h"
#include "drivers.h"

uint8_t g_error_last __attribute__ ((section (".noinit"))); // do not zero out on reset

void halt(uint8_t code) //{{{
{
	interrupt_disable();

	// remember last error
	g_error_last = code;
	
	reset();
} //}}}
