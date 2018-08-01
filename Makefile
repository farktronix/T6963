obj-m := t6963_graphics.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
CC = gcc

default:
	rm -f t6963_graphics.ko
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
	rmmod t6963_graphics.ko
	insmod t6963_graphics.ko
	rm -f /dev/lcd
	mknod /dev/lcd c `cat /proc/devices | grep t6963 | cut -d " " -f 1` 0

logo: logo.c
	$(CC) -o logo logo.c
