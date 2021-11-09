#!/bin/sh

aarch64-linux-gnu-gcc -c boot.S -o boot.o
aarch64-linux-gnu-gcc -ffreestanding -nostdlib -nostdinc -c kernel.c -o kernel.o
aarch64-linux-gnu-ld -nostdlib boot.o kernel.o -T link.ld -o kernel8.elf
