#!/bin/bash 

adb root
adb shell '
FLASH_FILE="/sys/class/leds/led:flash_1/brightness"
FLASH_INT_S=0.01

#echo "trace_event_clock_sync: name=battor regulator=8941_smbb_boost" > \
#  /sys/kernel/debug/tracing/trace_marker

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
	sleep $FLASH_INT_S
	i=$(($i + 1))
done
echo 0 > $FLASH_FILE
echo 0

#echo "trace_event_clock_sync: name=battor regulator=8941_smbb_boost" > \
#  /sys/kernel/debug/tracing/trace_marker
'
