#!/system/bin/sh

FLASH_FILE='/sys/class/leds/led:flash_1/brightness'
FLASH_INT_S=0.05

# disable charging
su -c 'echo 0xCA > /sys/kernel/debug/bq24192/INPUT_SRC_CONT'
su -c 'chmod 644 /sys/class/power_supply/usb/online'
su -c 'echo 0 > /sys/class/power_supply/usb/online'

# turn on kernel tracing
su -c "echo 0 > /sys/kernel/debug/tracing/tracing_on"
su -c "echo > /sys/kernel/debug/tracing/trace" # clear trace buffer
su -c "echo 1 > /sys/kernel/debug/tracing/events/regulator/enable"
su -c "echo 1 > /sys/kernel/debug/tracing/buffer_size_kb"
su -c "echo 1 > /sys/kernel/debug/tracing/tracing_on"

sleep 2

# drop sync signal with flash
su -c "chmod 777 $FLASH_FILE"
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

cat /sys/kernel/debug/tracing/trace | grep boost > /sdcard/flash_trace

# enable charging
su -c 'echo 0x4A > /sys/kernel/debug/bq24192/INPUT_SRC_CONT'
su -c 'echo 1 > /sys/class/power_supply/usb/online'
