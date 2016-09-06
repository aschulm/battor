<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**  *generated with [DocToc](https://github.com/thlorenz/doctoc)*

- [BattOr: Mobile device power monitor](#battor-mobile-device-power-monitor)
  - [Directory structure](#directory-structure)
  - [Installation](#installation)
    - [Software](#software)
    - [Systrace](#systrace)
    - [Firmware](#firmware)
  - [Usage](#usage)
    - [Desktop operation mode](#desktop-operation-mode)
    - [Portable operation mode](#portable-operation-mode)

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

### Desktop operation mode

**LED status codes:**
* Blinking *YELLOW*: Idle, waiting to start next trace.
* Blinking *RED*: Recording trace.
* Solid *RED*: Downloading trace.
* Blinking *GREEN*: Streaming trace.

To stream samples from the BattOr over USB, run the following on the command line.
Often you will want to redirect the output to a file.

    $ battor -s

To start buffering power measurements, run the following on the command line.
Once the BattOr has a blinking *RED* LED, it can be disconnected from USB so power can
be measured while on the move.

    $ battor -b

To end buffering and download the trace, run the following on the command line.
Often you will want to redirect the output to a file. The BattOr will have a
solid *RED* LED until the download is completed. Note that currently
downloading buffered power measurements takes approximately 1/4 of time that
the samples were buffered.

    $ battor -d

### Portable operation mode

**LED status codes:**
* Solid *GREEN*: Portable mode enabled.
* Blinking *YELLOW*: Idle, waiting to start next trace. Number of strobes indicates the next file number to write (e.g., two strobes indicates the next file is file #2).
* Blinking *RED* Recording trace. Number of strobes indicates file currently being written.
* Solid *RED*: Downloading trace.

To enter portable operation mode, hold the *Rec* button for 5 seconds. The
green LED will stay on indicating that the BattOr is in portable mode. You can
then tap the *Rec* button to start and stop recording traces.  As described
above, the strobes of the YELLOW LED indicates the number of the next file that
will be written, and the strobes of the RED LED indicates the number of the
file currently being written. When there are too many strobes for your keep a
good count, you can reset to file #1 by holding the *Rec* button for 5
seconds.

To download a trace file, run the following on the command line and fill in the
file number. The BattOr will have a solid *RED* LED until the download is
completed.

	$ battor -d <file number>
