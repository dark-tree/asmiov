#!/bin/env bash

txt=$(mktemp)
obj=$(mktemp)
elf=$(mktemp)
lnk=$(mktemp)

trap "rm -f $txt $obj $elf $lnk" EXIT

printf "$2" >> $txt

echo "SECTIONS { " >> $lnk
echo "    . = $1;" >> $lnk
echo "    .text : ALIGN(CONSTANT(MAXPAGESIZE)) { *(.text) }" >> $lnk
echo "    .data : ALIGN(CONSTANT(MAXPAGESIZE)) { *(.data) }" >> $lnk
echo "}" >> $lnk

echo "[1] nasm $txt"
nasm -f elf64 $txt -o $obj

echo "[2] link $obj"
ld -T $lnk -nostdlib -melf_x86_64 $obj -o $elf -e $1
strip $elf

echo "[3] dump $elf"
#readelf -lSWa $elf
objdump -Mintel,intel64 -xD $elf
