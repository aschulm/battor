#!su -c /system/bin/sh

FLASH_FILE='/sys/class/leds/led:flash_1/brightness'
FLASH_INT_S=0.01

# disable charging
echo 0xCA > /sys/kernel/debug/bq24192/INPUT_SRC_CONT
chmod 644 /sys/class/power_supply/usb/online
echo 0 > /sys/class/power_supply/usb/online

sleep 1

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

# enable charging
echo 0x4A > /sys/kernel/debug/bq24192/INPUT_SRC_CONT
echo 1 > /sys/class/power_supply/usb/online
