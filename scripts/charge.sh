#!/bin/bash

PRODUCT=`adb shell getprop ro.build.product`
PRODUCT=${PRODUCT::-1} # remove extra '\r'

SUCHECK=`adb shell 'ls /system/xbin/su >/dev/null 2>/dev/null; echo $?'`
SUCHECK=${SUCHECK::-1} # remove extra '\r'

function adbshell 
{
	cmd=$1
	if [ $SUCHECK = "0" ]
	then
		adb shell "su -c \"$cmd\""
	else
		adb shell "$cmd"
	fi
}

adb root >/dev/null 2>/dev/null

# enable charging
if [ $1 -eq 1 ]
then
	adbshell 'dumpsys battery reset'
	if [ $PRODUCT = "hammerhead" ]
	then
		adbshell '
		echo 0x4A > /sys/kernel/debug/bq24192/INPUT_SRC_CONT
		'
		exit
	fi
	# s6
	if [ $PRODUCT = "zerofltespr" ]
	then
		adbshell '
		chmod 644 /sys/class/power_supply/battery/test_mode
		echo 0 > /sys/class/power_supply/battery/test_mode
		chmod 644 /sys/class/power_supply/max77843-charger/current_now
		echo 9999 > /sys/class/power_supply/max77843-charger/current_now
		'
	fi
  if [ $PRODUCT = "flounder" ]
	then
		adbshell '
		echo D > /sys/bus/i2c/drivers/bq2419x/0-006b/input_cable_state
		'
  fi
fi

# disable charging
if [ $1 -eq 0 ]
then
	adbshell "dumpsys battery set usb 0"
	if [ $PRODUCT = "hammerhead" ]
	then
		adbshell '
		echo 0xCA > /sys/kernel/debug/bq24192/INPUT_SRC_CONT\
		'
	fi
	if [ $PRODUCT = "zerofltespr" ]
	then
		adbshell '
		chmod 644 /sys/class/power_supply/battery/test_mode
		echo 1 > /sys/class/power_supply/battery/test_mode
		chmod 644 /sys/class/power_supply/max77843-charger/current_now
		# current_now only says it goes down to 100, but a BattOr intercepted USB
		# cable indiciates < 10mA, so USB power appears to be sufficiently disabled
		echo 0 > /sys/class/power_supply/max77843-charger/current_now
		'
	fi
  if [ $PRODUCT = "flounder" ]
	then
		adb shell '
		echo "C" > /sys/bus/i2c/drivers/bq2419x/0-006b/input_cable_state
		'
  fi
fi
