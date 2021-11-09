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
make --no-print-directory -C nolibc \
	"CC=aarch64-rpi4-none-static-cc" \
	"FREESTANDING_CFLAGS=-I$(pwd)/nolibc/include -include _freestanding/overrides.h -I$(pwd)/openlibm/src -I$(pwd)/openlibm/include" \
	"SYSDEP_OBJS=rpi4.o"
aarch64-linux-gnu-gcc -nostdinc \
	-isystem $(aarch64-linux-gnu-gcc -print-file-name=include) \
	-I $(aarch64-linux-gnu-gcc -print-file-name=include) \
	-ffreestanding \
	-Wl,--build-id=none \
	-nostdlib \
	-Wl,-T,link.ld -l :abi.elf \
	-L$(pwd)/nolibc -lnolibc \
	-L$(pwd)/openlibm -lopenlibm -lgcc \
	-L$(pwd) \
	-z max-page-size=0x1000 \
	-static \
	-o kernel8.elf
