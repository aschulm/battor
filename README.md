# BattOr - Power monitor for smartphones and tablets

## Directory structure

* fw - Atmel XMega192A3 firmware for the BattOr hardware
* sw - PC software for configuring and installing BattOr

## Build and install instructions

### Software

#### Dependencies
* libftdi >1.0 (must build [from source](http://www.intra2net.com/en/developer/libftdi/download.php) for Ubuntu 14.04)

The software should build and run on Linux and OSX. However, if you are
using OSX 10.9 (Maverics) you will need to disable the OSX FTDI USB-to-serial
driver.

    $ ./bootstrap
    $ ./configure
    $ make
    
### Firmware

#### Dependencies
* [AVR 8-bit toolchain](http://www.atmel.com/tools/atmelavrtoolchainforlinux.aspx) >3.4

The firmware currently only builds on Linux.

    $ make
    $ make flash
