#include "common.h"
#include "error.h"
#include "interrupt.h"

volatile uint16_t g_interrupt;

void interrupt_init() //{{{
{
	g_interrupt = 0; // clear all interrupts
	int_enable(PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm); // enable the low priority interrupts
	sei(); // enable interrupts
} //}}}
