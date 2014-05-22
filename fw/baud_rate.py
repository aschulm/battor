#!/usr/bin/env python

CPU_FREQ = 16e6
CLK2X = False

mult = 8 if CLK2X else 16

from sys import stdin

print "Enter desired baud rate: ",
target_baud = int(stdin.readline())

closest_baud = 0
closest_bscale = 0
closest_bsel = 0

for bsel in range(0,4096):
	for bscale in range(-7,8):
		if (bscale >= 0):
			baud = CPU_FREQ/((2**bscale) * mult * (bsel + 1))
		else:
			baud = CPU_FREQ/(mult*(((2**bscale) * bsel) + 1))

		if (abs(baud - target_baud) < abs(baud - closest_baud)):
			closest_baud = baud
			closest_bscale = bscale
			closest_bsel = bsel

print "BAUD: %f BSCALE: %d BSEL: %d" % (closest_baud, closest_bscale, closest_bsel)
