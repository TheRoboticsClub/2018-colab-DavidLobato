#!/bin/bash

if [[ $# -ne 1 || !($1 =~ ^[0-9]+$) || $1 -lt 1 ]]; then
    echo "Argument must be a number between 1..1024"
    exit 1
fi

bytes=$1

#write ascending count 0..255
echo -n "80 00 " #write command + address 0
for i in $(seq 0 $(($bytes-1)))
do
    printf "%x" "$(($i % 256))"
    if [[ $i -lt $bytes ]]; then
        printf " "
    fi
done
printf "\n"

#read $bytes starting at 0 address
echo -n "00 00 " #write command + address 0
for i in $(seq 0 $(($bytes-1)))
do
    printf "0"
    if [[ $i -lt $bytes ]]; then
        printf " "
    fi
done
printf "\n"