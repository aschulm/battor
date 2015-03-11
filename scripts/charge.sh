#!/bin/bash

adb root
if [ $1 -eq 1 ]
then
	# enable charging
  adb shell '
	echo 0x4A > /sys/kernel/debug/bq24192/INPUT_SRC_CONT
	echo 1 > /sys/class/power_supply/usb/online
  '
else
	# disable charging
  adb shell '
	echo 0xCA > /sys/kernel/debug/bq24192/INPUT_SRC_CONT
	chmod 644 /sys/class/power_supply/usb/online
	echo 0 > /sys/class/power_supply/usb/online
  '
fi
