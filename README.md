# BattOr - smartphone power monitor

## Directory structure

* fw - Atmel XMega192A3 firmware for the BattOr hardware
* sw - PC software for configuring and installing BattOr

## Build and install instructions

### Firmware

To build the firmware you will need avr-binutils, avr-gcc, avr-libc, and avrdude.

    $ make
    $ make flash (hit the reset button on BattOr as you do this)

### Software

The software should build and run on Linux and OSX. However, if you are
using OSX 10.9 (Maverics) you will need to disable the OSX FTDI USB-to-serial
driver and install 
[the VCP driver from FTDI](http://www.ftdichip.com/Drivers/VCP.htm).
To do so, follow the OSX 10.9 install instructions 
[from FTDI](http://www.ftdichip.com/Support/Documents/AppNotes/AN_134_FTDI_Drivers_Installation_Guide_for_MAC_OSX.pdf).

    $ ./bootstrap
    $ ./configure
    $ make
