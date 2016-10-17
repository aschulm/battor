import serial
from time import sleep

s = serial.Serial('/dev/ttyACM0', 19200, timeout=1)

s.write('gpio set 0\r');
sleep(2);
s.write('gpio clear 0\r');
