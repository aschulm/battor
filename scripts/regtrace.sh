#!/system/bin/sh

if [ $1 -eq 1 ]
	# turn on kernel tracing
	echo 0 > /sys/kernel/debug/tracing/tracing_on
	echo > /sys/kernel/debug/tracing/trace # clear trace buffer
	echo 1 > /sys/kernel/debug/tracing/events/regulator/enable
	echo 1 > /sys/kernel/debug/tracing/buffer_size_kb
	echo 1 > /sys/kernel/debug/tracing/tracing_on
else
	echo 0 > /sys/kernel/debug/tracing/tracing_on
	cat /sys/kernel/debug/tracing/trace > /sdcard/flash_trace
fi
