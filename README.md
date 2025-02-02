# SiSFSB : DOS FSB utility for SiS chipsets

SiSFSB is a DOS utility for changing the FSB/SDRAM/PCI clocks on motherboards based on SiS-based motherboards.

It is heavily inspired by [VIAFSB](https://github.com/enaiel/viafsb) (many thanks to its author Enaiel for contributing such awesome tool), but written from scratch in C++17.

# Supported hardware

## Chipsets

- SiS 540 / SiS 630 (Only tested on SiS 540)

## PLLs

- W83194R-630A (Winbond)

# How to use

- `SISFSB.EXE` and `CWSDPMI.EXE` need to be in the same directory
- Run sisfsb:
```
sisfsb -pll <PLL> -fsb <FSB>/<SDRAM>/<PCI>
```

For example:
```
sisfsb -pll W83194R_630A -fsb 100.00/100.00/33.00
```

# Build from source

You can use the [DJGPP](http://www.delorie.com/djgpp) toolchain for native DOS C++ development.
I used the following files:

```
unzip32.exe         to unzip the zip files         95 kb

v2/copying.dj       DJGPP Copyright info            3 kb
v2/djdev205.zip     DJGPP Basic Development Kit   2.4 mb
v2/faq230b.zip      Frequently Asked Questions    664 kb
v2/readme.1st       Installation instructions      23 kb

v2gnu/bnu2351b.zip  Basic assembler, linker       6.0 mb
v2gnu/gcc930b.zip   Basic GCC compiler           34.1 mb
v2gnu/gdb801b.zip   GNU debugger                  4.4 mb
v2gnu/gpp930b.zip   C++ compiler                 15.4 mb
v2gnu/mak44b.zip    Make (processes makefiles)    473 kb
```

- Extract them into `djgpp` directory like so: `unzip *.zip -d djgpp`.
- Set up the `PATH` environmental variables, as listed in the DJGPP instructions.
- Now place your sisfsb directory next to `djgpp`, and you can now run:

```
make -C sisfsb/src
```

The binary should be create in the build directory (`sisfsb/src/build` for DOS).

## DOSBox-X Build
For convenience I am building the project in [DOSBox-X](https://dosbox-x.com/) and I have set the following in by dosbox configuration file (in `.config/dosbox-x/dosbox-x-<version>.conf` on Linux):

```
[dos]
lfn=true

[cpu]
cycles=max

[autoexec]
set PATH=C:\DJGPP\BIN;%PATH%
set DJGPP=C:\DJGPP\DJGPP.ENV
```

Notes:
- `lfn=true` turns on Long Filename support, which is needed due to the long filenames in C++ standard library. You can the version with: `dosbox-x --version`.
- It is *very* slow, it takes several minutes to build, but it works!
- For local prototyping on Linux you can use `make OS=LINUX` and `make clean OS=LINUX` which will build the objects and the final binary in `build_linux/`.

# Licence
GPL-2.0
