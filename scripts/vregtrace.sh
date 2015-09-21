#!/bin/bash

SUCHECK=`adb shell 'ls /system/xbin/su >/dev/null 2>/dev/null; echo $?'`
SUCHECK=`echo $SUCHECK | rev | cut -c 2- | rev` # remove extra '\r'

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

adb root
if [ $1 -eq 1 ]
then
	adbshell '
	echo 1 > /sys/kernel/debug/tracing/events/regulator/enable
	'
else
	adbshell '
	echo 0 > /sys/kernel/debug/tracing/events/regulator/enable
	'
fi
