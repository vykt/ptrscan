#! /bin/sh

# this example produces a scan for <addr> targeting example_proc

gdb --args ./build/scan_debug -c -v -p 0x100 -l 4 -q -t 1 -a <addr> -w DEBUGOUT.pscan example_proc
