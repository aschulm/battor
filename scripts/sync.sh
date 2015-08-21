#!/bin/bash 

SUCHECK=`adb shell 'ls /system/xbin/su >/dev/null 2>/dev/null; echo $?'`
SUCHECK=`echo $SUCHECK | rev | cut -c 2- | rev`  # remove extra '\r'

function adbshell 
{
	cmd=$1
	if [ $SUCHECK = "0" ]
	then
		adb shell "su -c \"$cmd\""
    echo "SU"
	else
		adb shell "$cmd"
	fi
}

echo '
FLASH_FILE="/sys/class/leds/led:flash_1/brightness"

echo "trace_event_clock_sync: name=battor regulator=8941_smbb_boost" > \
  /sys/kernel/debug/tracing/trace_marker

# drop sync signal with flash
chmod 777 $FLASH_FILE
i=0
echo 1 > $FLASH_FILE
echo -n 1
while [ "$i" -lt 64 ]
do
	randbit=$(($RANDOM % 2))
	echo $(($randbit * 255)) > $FLASH_FILE
  echo -n $randbit
	sleep 0.01
	i=$(($i + 1))
done
echo 0 > $FLASH_FILE
echo 0

echo "trace_event_clock_sync: name=battor regulator=8941_smbb_boost" > \
  /sys/kernel/debug/tracing/trace_marker
' > /tmp/sync.sh

adb push /tmp/sync.sh /sdcard/sync.sh 2>/dev/null >/dev/null
adbshell 'chmod 777 /sdcard/sync.sh'
adbshell '/system/bin/sh /sdcard/sync.sh'
