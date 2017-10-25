# The Pokitto port of Micropython

This port is intended for the Pokitto device which is based on LPC11U68 MCU.

## Building for an LPC11U68 MCU

Building under Cygwin
---------------------

Install following packages using cygwin's setup.exe:

* mingw64-i686-gcc-core
* mingw64-x86_64-gcc-core
* make

The Makefile has the ability to build for a Cortex-M0+ CPU, and also can enable a UART
for communication.  To build:

    $ make CROSS=1 lib

