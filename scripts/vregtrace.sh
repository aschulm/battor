#!/system/bin/sh

if [ $1 -eq 1 ]
	echo 1 > /sys/kernel/debug/tracing/events/regulator/enable
else
	echo 0 > /sys/kernel/debug/tracing/events/regulator/enable
fi
