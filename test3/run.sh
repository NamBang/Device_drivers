#!/bin/bash
make clean
make mrproper
make ARCH=arm CROSS_COMPILE=/home/nambang/Documents/DriverLinux/Document/source_code/gcc-linaro-arm/gcc-linaro/bin/arm-linux-gnueabi- sun8iw7p1smp_defconfig
make 


