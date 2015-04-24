# BattOr - Power monitor for smartphones and tablets

## Directory structure

* fw - Atmel XMega192A3 firmware for the BattOr hardware
* sw - PC software for configuring and installing BattOr
* systrace - Fork of Android systrace for visualizing power measurments synced with phone events

## Build and install instructions

### Software

#### Dependencies
* libboost-all-dev >= 1.54 (for libftdi)
* libusb-dev >= 1.0 (for libftdi)
* libftdi >= 1.0 (must build [from source](http://www.intra2net.com/en/developer/libftdi/download.php) for Ubuntu 14.04)

The software should build and run on Linux and OSX. However, if you are
using OSX 10.9 (Maverics) you will need to disable the OSX FTDI USB-to-serial
driver.

    $ ./bootstrap
    $ ./configure
    $ make
    $ sudo make install

To run BattOr without root on Linux you will need to add the BattOr to your udev rules.

    $ sudo echo 'SUBSYSTEMS=="usb", ATTRS{idVendor}=="0403", GROUP="plugdev", MODE="0660"' > /etc/udev/rules.d/99-libftdi.rules"
    $ sudo udevadm control --reload-rules
    $ sudo udevadm trigger
    
### Systrace

#### Dependencies
* Chrome
* Android adb

Systrace collects trace data from BattOr and Android and produces an interactive timeline view of the data in html using [trace-viewer](http://github.com/google/trace-viewer).

### Firmware

#### Dependencies
* [AVR 8-bit toolchain](http://www.atmel.com/tools/atmelavrtoolchainforlinux.aspx) >3.4

The firmware currently only builds on Linux. The BattOr software must be in $PATH to flash the firmware.

    $ make
    $ make flash
