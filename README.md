# ptrscan

<p align="center">
    <img src="ptrscan.png" width="150" height="150">
</p>

### NOTE:

**ptrscan**'s dependencies are undergoing major testing & minor refactors. Prebuilt binaries & static builds are coming. To get **ptrscan** running in the meantime, install the latest release, [libcmore v0.0.3](https://gitub.com/vykt/cmore/releases/tag/0.0.3), and [liblain v1.0.2](https//github.com/vykt/liblain/releases/tag/1.0.2).


### ABOUT:

Pointer Scanner (**ptrscan**) is a dynamic analysis utility for discovering related pointers within a target process. 

Because pointers are typically stored as parts of structures, **ptrscan** also discovers structures and their relationships, which at a high level reveals the design of your target.

**ptrscan** is able to *verify* previously identified pointer chains by attempting to follow them through different instances of the same program, eliminating false positives.

In addition to assisting with dynamic analysis, select pointer chains produced by **ptrscan** can be used to reliably navigate the memory of a target process from an external process.



### DEPENDENCIES:

**ptrscan** links the following at runtime:

- [liblain](https://github.com/vykt/liblain)
- [libcmore](https://github.com/vykt/libcmore)

**ptrscan** can optionally use this LKM to scan through a hidden kernel interface:

- [lain.ko](https://github.com/vykt/lain.ko)



### INSTALLATON:

Fetch the repo:
```
$ git clone https://github.com/vykt/ptrscan
```

Build:
```
$ cd ptrscan
$ make ptrscan build=release
```

Install:
```
# make install
```

Install additional markdown documentation:
```
# make install_doc
```

To uninstall:
```
# make uninstall
```



### DOCUMENTATION:

After installing **ptrscan**, see `man 1 ptrscan`. Alternatively, markdown documentation is available at `./doc/md/ptrscan.md`. 



### EXAMPLES:

See `./doc/md/ptrscan.md` for a walkthrough of a typical use case.
