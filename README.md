# BattOr - smartphone power monitor

## Directory structure
fw - Atmel XMega192A3 firmware for the BattOr hardware
sw - PC software for configuring and installing BattOr

## Build and install instructions

The fw and software should build and run on Linux and OSX. However, if you are
using OSX 10.9 (Maverics) you will need to disable the OSX FTDI USB-to-serial
driver and install the one from FTDI.

Follow the OSX 10.9 instructions [from FTDI](http://www.ftdichip.com/Support/Documents/AppNotes/AN_134_FTDI_Drivers_Installation_Guide_for_MAC_OSX.pdf).

    fw:
    $ ./bootstrap
    $ ./configure
    $ make

    sw:
    $ make
    $ make flash (to flash over USB)
