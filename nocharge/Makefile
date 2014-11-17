KERNEL_DIR=~/Projects/android/msm

obj-m := nocharge.o
PWD := $(shell pwd)

default:
	$(MAKE) ARCH=arm SUBARCH=arm CROSS_COMPILE=arm-eabi- -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules
clean:
	$(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(PWD) clean
