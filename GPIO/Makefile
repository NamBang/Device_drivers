obj-m := led.o

KDIR=/home/nambang/Documents/DriverLinux/buildKernel/slove3/linux-stable


TOOLCHAIN =/home/nambang/Documents/DriverLinux/buildKernel/OrangePiH3/OrangePiH3_toolchain/bin/arm-linux-gnueabi-

TARGET = arm

PWD := $(shell pwd)


all:
	make -C $(KDIR) M=$(PWD) ARCH=$(TARGET) CROSS_COMPILE=$(TOOLCHAIN) modules

clean:
	make -C $(KDIR) M=$(PWD) ARCH=$(TARGET) CROSS_COMPILE=$(TOOLCHAIN) clean

