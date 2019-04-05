
obj-m := hello.o

KDIR = /home/nambang/Documents/DriverLinux/Document/source_code/linux-4_19_20

TOOLCHAIN = /home/nambang/Documents/DriverLinux/Document/source_code/gcc-linaro-arm/gcc-linaro/bin/arm-linux-gnueabi-

TARGET = arm

PWD := $(shell pwd)


all:
	make -C $(KDIR) M=$(PWD) ARCH=$(TARGET) CROSS_COMPILE=$(TOOLCHAIN) modules

clean:
	make -C $(KDIR) M=$(PWD) ARCH=$(TARGET) CROSS_COMPILE=$(TOOLCHAIN) clean
