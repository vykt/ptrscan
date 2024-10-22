#! /bin/sh
gdb --args ../build/ptr/ptrscan -r DEBUG.pscan -T -p -n -a $1 -s 0x80 target
