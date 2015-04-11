#include <avr/io.h>

#include "reset.h"

void reset()
{
	CCP = CCP_IOREG_gc;
	RST.CTRL = RST_SWRST_bm;
}
