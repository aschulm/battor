#!/bin/bash

adb root
if [ $1 -eq 1 ]
then
	adb shell '
	echo 1 > /sys/kernel/debug/tracing/events/regulator/enable
	'
else
	adb shell '
	echo 0 > /sys/kernel/debug/tracing/events/regulator/enable
	'
fi
