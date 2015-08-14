# Name: Makefile
# Author: Aaron Schulman
# Copyright: <insert your copyright message here>
# License: Apache License 2.0
 
VERSION = 1.0

CFLAGS     = -mmcu=atxmega192a3u -DF_CPU=32000000 -Wall -O2 -DDEBUG #-DSAMPLE_INC #-DSAMPLE_ZERO #-gdwarf-2 #-DDEBUG #-DGPIO_SAMPLE_WRITE_DONE=1 #-DGPIO_DMA_INT=0
OBJECTS    = drivers/led.o drivers/clock.o drivers/uart.o drivers/adc.o drivers/dma.o drivers/nvm.o drivers/timer.o drivers/int.o drivers/spi.o drivers/pot.o drivers/gpio.o drivers/sd.o drivers/mux.o drivers/reset.o drivers/eeprom.o drivers/sram.o blink.o main.o drivers.o interrupt.o error.o control.o samples.o store.o
FUSES      = -U hfuse:w:0x99:m -U lfuse:w:0xff:m

DEBUG_PORT = 4242

USBDEV = /dev/ttyUSB0

COMPILE = avr-gcc $(CFLAGS) 

# symbolic targets:
all:	main.hex

.c.o:
	$(COMPILE) -c $< -o $@

.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@
# "-x assembler-with-cpp" should not be necessary since this is the default
# file type for the .S (with capital S) extension. However, upper case
# characters are not always preserved on Windows. To ensure WinAVR
# compatibility define the file type manually.

.c.s:
	$(COMPILE) -S $< -o $@

flash: all
	@battor -k > /dev/null
	@modprobe ftdi-sio
	@echo 0403 77E0 > /sys/bus/usb-serial/drivers/ftdi_sio/new_id
	avrdude -e -p x192a3u -c avr109 -b 115200 -P $(USBDEV) -U flash:w:main.hex
	@modprobe -r ftdi-sio

flash_mkII: all
	avrdude -p x192a3u -c avrispmkII -P usb -U flash:w:main.hex

flash_ICE: all
	avrdude -p x192a3u -c atmelice_pdi -B0.5 -U flash:w:main.hex

fuse:
	avrdude -p x192a3u -c avrispmkII -P usb -U $(FUSES)

# Xcode uses the Makefile targets "", "clean" and "install"
install: flash

# if you use a bootloader, change the command below appropriately:
load: all
	bootloadHID main.hex

clean:
	rm -f main.hex main.elf $(OBJECTS)

# file targets:
main.elf: $(OBJECTS)
	$(COMPILE) -o main.elf $(OBJECTS)

main.hex: main.elf
	rm -f main.hex
	avr-objcopy -j .text -j .data -O ihex main.elf main.hex

# If you have an EEPROM section, you must also create a hex file for the
# EEPROM and add it to the "flash" target.

# Targets for code debugging and analysis:
disasm:	main.elf
	avr-objdump -d main.elf

cpp:
	$(COMPILE) -E main.c

gdb-config: 
	@rm -f .gdbinit
	@echo define reset >> .gdbinit
	@echo SIGNAL SIGHUP >> .gdbinit
	@echo end >> .gdbinit 
	@echo file main.elf >> .gdbinit
	@echo target remote localhost:$(DEBUG_PORT)  >> .gdbinit
	@echo break main >> .gdbinit

debug: gdb-config main.elf
	@echo Starting AVaRICE - Press enter when "waiting to connect" message displays.
	avarice -j usb --mkII --erase --program --file \
		main.elf localhost:$(DEBUG_PORT)
	@avr-gdb --command=.gdbinit

dist-gzip:
	tar cvfz battor-atmel-$(VERSION).tar.gz *.h *.c Makefile

console:
	picocom -b 115200 $(USBDEV)
