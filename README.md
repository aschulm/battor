<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**  *generated with [DocToc](https://github.com/thlorenz/doctoc)*

- [BattOr: Power monitor for smartphones and laptops](#battor-power-monitor-for-smartphones-and-laptops)
  - [Directory structure](#directory-structure)
  - [Installation](#installation)
    - [Software](#software)
    - [Systrace](#systrace)
    - [Firmware](#firmware)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

# BattOr: Mobile device power monitor

## Directory structure

* sw - PC software for collecting power measurements from the BattOr
* fw - Atmel XMega192A3 firmware for the BattOr hardware
* systrace - Fork of Android systrace for visualizing power measurments synced with phone events

## Installation

### Software

The software should build and run on most variants of Linux. The build and install instructions are as follows:

    $ ./bootstrap
    $ ./configure
    $ make
    $ sudo make install

To run BattOr without root on Linux you will need to add your user to the "dialout" group. You need to logout and login for these settings to take effect.

    $ sudo usermod -a -G dialout <userName>
    
### Systrace

**Dependencies**
* Chrome
* Android adb

Systrace collects trace data from BattOr and Android and produces an interactive timeline view of the data in html using [trace-viewer](http://github.com/google/trace-viewer).

### Firmware

**Dependencies**
* BattOr software installed
* [AVR 8-bit toolchain](http://www.atmel.com/tools/atmelavrtoolchainforlinux.aspx) >3.4
* avrdude

The firmware currently only builds on Linux. The BattOr software must be in $PATH to flash the firmware.

    $ make
    $ make flash

## Usage

### LED status codes
* Fast blinking yellow: In bootloader
* Slow blinking yellow: Idle
* Blinking red: Buffering to SD card
* Solid red: Downloading from SD card
* Blinking green: Streaming over USB
