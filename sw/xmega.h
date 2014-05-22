#ifndef XMEGA_H
#define XMEGA_H

typedef enum XMEGA_CLKDIV_enum
{
	XMEGA_CLKDIV_OFF_gc = (0x00<<0),
	XMEGA_CLKDIV_1_gc = (0x01<<0),
	XMEGA_CLKDIV_2_gc = (0x02<<0),
	XMEGA_CLKDIV_4_gc = (0x03<<0),
	XMEGA_CLKDIV_8_gc = (0x04<<0),
	XMEGA_CLKDIV_64_gc = (0x05<<0),
	XMEGA_CLKDIV_256_gc = (0x06<<0),
	XMEGA_CLKDIV_1024_gc = (0x07<<0)
} XMEGA_CLKDIV_t;

#endif
