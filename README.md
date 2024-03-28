<p align="center">
    <img src="logo.png">
</p>

# ptrscan

### ABOUT:

Ptrscan is an implementation of a pointer scanner following the ideas of Cheat Engine's own pointer scanner implementation. Ptrscan was written from scratch to run natively on Linux - no more running CE through wine. 

[What is a pointer scanner?](https://guidedhacking.com/threads/cheat-engine-how-to-pointer-scan-with-pointermaps.9739/)

---

### DEPENDENES:

Ptrscan dynamically links [libpwu](https://github.com/vykt/libpwu). Release 0.1.4 is required. Visit the page and follow installation instructions.

---

### INSTALLAON:

Fetch the repo:
```
$ git clone https://github.com/vykt/ptrscan
```

Generate build files:
```
$ cd ptrscan && ./buildgen.sh
```

Build the release:
```
$ cd build && make scan
```

Check the install script & install:
```
$ cd .. && sudo ./install.sh
```

---

### EXAMPLES:

Using ptrscan is covered in the ptrscan manpage:
```
$ man ptrscan
```

1) Pointer scan process example\_proc for address 0x55134a90f080 (-a) and save the results to first\_map.pscan (-w):
```
ptrscan -a 0x55134a90f080 -w first_map.pscan example_proc
```

2) Verify the pointer chains (-x) in first\_map.pscan (-r) to check that they arrive at address 0x55431bea1080 (-a). Output the new results to second\_map.pscan (-w).
```
ptrscan -x -a 0x55431bea1080 -r first_map.pscan -w second_map.pscan example_proc
```

3) Using the default terminal interface (-c), carry out an aligned (-q) pointer scan with 0x500 lookback (-p) and the depth of 4 levels (-l). Use 4 threads (-t). Report on the progress of the scan (-v). Output pointer chains to the third\_map.pscan file. Carry out the scan on the example\_proc process.
```
ptrscan -c -v -q -p 0x500 -l 4 -t 4 -a 0x7fffba434000 -w third_map.pscan example_proc
```

---

### FUTURE CONSIDERATIONS:

An alternative ncurses interface is planned. For any other feature requests or bugs please open an issue.
