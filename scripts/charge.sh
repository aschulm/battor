#!/bin/bash

PRODUCT=`adb shell getprop ro.build.product`
PRODUCT=${PRODUCT::-1} # remove extra '\r'

adb root >/dev/null 2>/dev/null

# enable charging
if [ $1 -eq 1 ]
then
	adb shell 'dumpsys battery set usb 1'
	if [ $PRODUCT = "hammerhead" ]
	then
		adb shell '
		echo 0x4A > /sys/kernel/debug/bq24192/INPUT_SRC_CONT
		echo 1 > /sys/class/power_supply/usb/online
		'
	fi
	# s6
	if [ $PRODUCT = "zerofltespr" ]
	then
		adb shell '
		chmod 644 /sys/class/power_supply/battery/test_mode
		echo 0 > /sys/class/power_supply/battery/test_mode
		chmod 644 /sys/class/power_supply/max77843-charger/current_now
		echo 9999 > /sys/class/power_supply/max77843-charger/current_now
		'
	fi
fi

# disable charging
if [ $1 -eq 0 ]
then
	adb shell 'dumpsys battery set usb 0'
	if [ $PRODUCT = "hammerhead" ]
	then
  	adb shell '
		echo 0xCA > /sys/kernel/debug/bq24192/INPUT_SRC_CONT
		chmod 644 /sys/class/power_supply/usb/online
		echo 0 > /sys/class/power_supply/usb/online
  	'
	fi
	if [ $PRODUCT = "zerofltespr" ]
	then
		adb shell '
		chmod 644 /sys/class/power_supply/battery/test_mode
		echo 1 > /sys/class/power_supply/battery/test_mode
		chmod 644 /sys/class/power_supply/max77843-charger/current_now
		echo 0 > /sys/class/power_supply/max77843-charger/current_now
		'
	fi
fi
