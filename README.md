<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**  *generated with [DocToc](https://github.com/thlorenz/doctoc)*

- [BattOr - Power monitor for smartphones and tablets](#battor---power-monitor-for-smartphones-and-tablets)
  - [Directory structure](#directory-structure)
  - [Build and install instructions](#build-and-install-instructions)
    - [Software](#software)
      - [Build](#build)
      - [User permission](#user-permission)
    - [Systrace](#systrace)
      - [Dependencies](#dependencies)
    - [Firmware](#firmware)
      - [Dependencies](#dependencies-1)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

# BattOr - Power monitor for smartphones and tablets

## Directory structure

* fw - Atmel XMega192A3 firmware for the BattOr hardware
* sw - PC software for configuring and installing BattOr
* systrace - Fork of Android systrace for visualizing power measurments synced with phone events

## Build and install instructions

### Software

The software can stream at max 10 ksps over USB and store at 1 ksps to the board's SD card.

#### Build
The software should build and run on Linux and OSX. However, if you are
using OSX 10.9 (Maverics) you will need to disable the OSX FTDI USB-to-serial
driver.

    $ ./bootstrap
    $ ./configure
    $ make
    $ sudo make install

#### User permission
To run BattOr without root on Linux you will need to add your user to the "dialout" group. You need to logout and login for these settings to take effect.

    $ sudo usermod -a -G dialout <userName>
    
### Systrace

#### Dependencies
* Chrome
* Android adb

Systrace collects trace data from BattOr and Android and produces an interactive timeline view of the data in html using [trace-viewer](http://github.com/google/trace-viewer).

### Firmware

#### Dependencies
* BattOr software installed
* [AVR 8-bit toolchain](http://www.atmel.com/tools/atmelavrtoolchainforlinux.aspx) >3.4
* avrdude

The firmware currently only builds on Linux. The BattOr software must be in $PATH to flash the firmware.

    $ make
    $ make flash
