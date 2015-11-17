#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "params.h"
#include "samples.h"
#include "xmega.h"
#include "control.h"
#include "uart.h"

extern char g_verb;

#define verb_printf(fmt, ...) \
	            do { if (g_verb > 0) fprintf(stderr, fmt, __VA_ARGS__); } while (0)
#define vverb_printf(fmt, ...) \
	            do { if (g_verb > 1) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

#endif
