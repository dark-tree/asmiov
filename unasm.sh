#!/bin/env bash

txt=$(mktemp)
obj=$(mktemp)
elf=$(mktemp)

trap "rm -f $txt $obj $elf" EXIT

echo "SECTION .CODE" >> $txt
echo "GLOBAL _start" >> $txt
echo >> $txt
echo "_start:" >> $txt
echo "$@" >> $txt

echo "[1] nasm $txt"
nasm -f elf32 $txt -o $obj

echo "[2] link $obj"
ld -melf_i386 $obj -o $elf

echo "[3] dump $elf"
objdump -Mintel -D $elf
