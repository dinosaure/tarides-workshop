#!/bin/bash

export PATH="$PATH:$(pwd)"

aarch64-linux-gnu-gcc -c boot.S -o boot.o
aarch64-linux-gnu-gcc \
	-isystem $(aarch64-linux-gnu-gcc -print-file-name=include) \
	-DPRINTF_INCLUDE_CONFIG_H \
	-ffreestanding -nostdlib -nostdinc -c kernel.c -o kernel.o
aarch64-linux-gnu-gcc \
	-isystem $(aarch64-linux-gnu-gcc -print-file-name=include) \
	-DPRINTF_INCLUDE_CONFIG_H \
	-ffreestanding -nostdlib -nostdinc -c printf.c -o printf.o
aarch64-linux-gnu-ld \
	-r -nostdlib boot.o kernel.o printf.o -o abi.elf
make --no-print-directory -C openlibm "CC=aarch64-rpi4-none-static-cc" libopenlibm.a
aarch64-linux-gnu-ld -nostdlib abi.elf -L$(pwd)/openlibm -lopenlibm -T link.ld -o kernel8.elf
