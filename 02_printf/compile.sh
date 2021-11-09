#!/bin/sh

aarch64-linux-gnu-gcc -c boot.S -o boot.o
aarch64-linux-gnu-gcc \
	-isystem $(aarch64-linux-gnu-gcc -print-file-name=include) \
	-DPRINTF_INCLUDE_CONFIG_H \
	-ffreestanding -nostdlib -nostdinc -c kernel.c -o kernel.o
aarch64-linux-gnu-gcc \
	-isystem $(aarch64-linux-gnu-gcc -print-file-name=include) \
	-DPRINTF_INCLUDE_CONFIG_H \
	-ffreestanding -nostdlib -nostdinc -c printf.c -o printf.o
aarch64-linux-gnu-ld -nostdlib boot.o kernel.o printf.o -T link.ld -o kernel8.elf
