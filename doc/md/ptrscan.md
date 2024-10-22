### NAME
Pointer chain scanner (ptrscan)


### SYNOPSIS

**ptrscan** **-a** [*ADDR*] [*TARGET_PROCESS*]  
**ptrscan** **-a** [*ADDR*] **-A** [*ALIGNMENT*] **-o** [*OUTPUT_FILE*] [*TARGET_PROCESS*]  
**ptrscan** **-a** [*ADDR*] **-r** [*INPUT_FILE*] **-x** **-o** [*OUTPUT_FILE*] [*TARGET_PROCESS*]

### DESCRIPTION

Given a target process and a target address, **ptrscan** will scan all virtual memory areas with read & write permissions and locate pointer chains leading to the target address.
The output produced by **ptrscan** can be used to locate and/or navigate data structures of a target process.

**ptrscan** can save the discovered pointer chains to a *.pscan* file. **pscan** can then verify the saved pointer chains against a different instance of the target process to eliminate the vast majority of false positives.

A pointer chain consists of a starting address, denoted by the basename of a backing object, followed by a series of offsets. To resolve backing object addresses, refer to the appropriate */proc/<pid>/maps* entries.


### OPTIONS

**-w** *OUTPUT_FILE*, **--output-file**=*OUTPUT_FILE*  
    Save pointer chains to a file. Works for both newly discovered, or verified pointer chains.

**-r** *INPUT_FILE*, **--input-file**=*INPUT_FILE*  
    Read pointer chains from a file. Without the **-x** option, this simply prints saved pointer chains to *stdout*.

**-T**, **--ui-term**  
    [*default*] Use the terminal interface.

**-N**, **--ui-ncurses**  
    [**not implemented**] Use the ncurses interface.

**-p**, **--iface-procfs**  
    [*default*] Use the *procfs* interface from **liblain**. This will use */proc/[pid]/mem* and */proc/[pid]/maps* for accessing memory of the target. For more information, see **man 3** *liblain_iface*.

**-k**, **--iface-lainko**  
Use the *lain.ko* interface from **liblain**. This will use the **lain.ko** kernel driver for accessing memory of the target. To use this **liblain** interface, you will need to have **lain.ko** already loaded into your kernel.

**-c**, **--colour**  
Enable coloured output for printing results.

**-n**, **--no-colour**  
[*default*] Disable coloured output for printing results.

**-v**, **--verbose**  
Report on progress for each thread at each depth level. This floods the terminal.

**-A** *ALIGNMENT*, **--alignment**=*ALIGNMENT*  
Boundary to which pointers are aligned. For example: If set to 4, **ptrscan** will only scan for pointers at 0x4, 0x8, 0xc, [...].

**-b** *BIT_WIDTH*, **--byte-width**=*BIT_WIDTH*  
Maximum number of bytes the target's architecture can process simultaneously. For example: On x86 byte width is 4, on x86\_64 byte idth is 8.

**-a** *ADDRESS*, **--target-address**=*ADDRESS*  
Address to pointer scan for.

**-s** *SIZE*, **--max-struct-size**=*SIZE*  
The maximum size structure to accept when performing the scan. For example: If your target address is 0x800, and **ptrscan** finds a pointer to 0x600, *SIZE* will need to be at least 0x200 for this pointer to be added to the chain.

**-d** *DEPTH*, **--max-depth**=*DEPTH*  
The maximum depth for this scan.

**-t** *THREADS*, **--threads**=*THREADS*  
Number of threads to use for the scan.

**-S** *FMT*, **--extra-static-regions**=*FMT*  
In addition to *[stack]* and .bss, treat backing objects included in *FMT* also as static. The format for *FMT* is:  
  
    <basename 1>,0:<basename 2>,0:<basename n>,0  
  
Where \<basename x> is the base name of the backing object to treat as static.

**-R** *FMT*, **--exclusive-rw-regions**=*FMT*  
Instead of scanning every memory area with read & write permissions, scan only the areas listed in *FMT*. The format for *FMT* is:  
  
    <basename 1>,0:<basename 2>,0:<basename n>,0  
  
Where \<basename x> is the base name of the backing object to exclusively scan. Note that *[stack]*, *[heap]*, and .bss are always scanned.

**-O** *PRESETS*, **--preset-offsets**=*PRESETS*  
Define the first preset offsets from the target address. The format for *PRESETS* is:

    <offset 1>,<offset 2>,<offset n>

Where \<offset x> is an offset supplied either in decimal or hexadecimal form ('0x' prefix required).

Preset offsets are specified in reverse order. For example: Preset offsets of 0x20,0x10,0x80 will result in a chain [...] 0x80 0x10 0x20.  

**-x**, **--verify**  
Verify the pointer chains read with **-r**. This option will cause **ptrscan** to follow each chain in the input file and verify that it arrives at the correct address, specified with **-a**.


### EXAMPLES

The following two examples follow the most common use case. Note that the addresses used in these examples are not realistic.

**ptrscan** **-a** *0x1000* **-b** *8* **-A** *2* **-s** 0x200 **-d** *4* **-t** *6* **-w** *first.pscan* *1337*

Pointer scan for address *0x1000* (**-a**). Set architecture's byte width to *8* (**-b**). Set pointer alignment in the target to *2* (**-A**). Accept structure sizes up to *0x200* bytes (**-s**). Scan up to the depth of *4* levels (**-d**). Use *6* threads for the scan (**-t**). Save the discovered pointer chains to *first.pscan* (**-w**). The target process has a PID of *1337*.

**ptrscan** **-a** *0x2000* **-r** *first.pscan* **-x** **-w** *verified.pscan* *target*.

Set the target address to *0x2000* (**-a**). Read a previous pointer scan from *first.pscan* (**-r**). Verify this scan (**-x**). Save the verified pointer chains to *verified.pscan* (**-w**). The target process is called *target* (**ptrscan** can resolve the PID for you).


### AUTHOR

Written by vykt.


### SEE ALSO
**ptrwatch** can be used to watch pointerchains found with **ptrscan** in real time.

You can use **liblain** to write tools that make use of pointer chains found with **ptrscan**. For bypassing countermeasures with **liblain**, see **lain.ko**.
