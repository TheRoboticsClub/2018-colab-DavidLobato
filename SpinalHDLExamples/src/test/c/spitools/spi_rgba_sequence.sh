#!/bin/bash

i=0
while true; do
    printf "%x\n" "$(($i % 8))"
    i=$(($i+1))
    sleep 0.1
done
