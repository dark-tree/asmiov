#!/bin/env bash

txt=$(mktemp)
bin=$(mktemp)

trap "rm -f $txt $bin" EXIT

echo "bits 64" >> $txt
echo >> $txt
echo "$@" >> $txt

if nasm -O0 $txt -o $bin; then
    xxd $bin
fi
