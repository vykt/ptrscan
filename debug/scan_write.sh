#! /bin/sh
../build/ptr/ptrscan -w DEBUG.pscan -T -p -n -A 4 -b 8 -a $1 -s 0x80 -d 4 -t 1 target
