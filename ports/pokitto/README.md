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

    make CROSS=1 lib
    
Pokitto related configuration
-----------------------------

In mpconfigport.h there are these defines:

- #define POKITTO_USE_HIRES_BUFFER
  - Tells if Pokitto is using hi-res (220x176x2bpp) or lo-res (110x88x4bpp) buffer.
  - This affects to the python GC heap size. The lo-res buffer takes less memory so the GC heap size can be larger

- #define POKITTO_USE_WIN_SIMULATOR
  - This tells if we are building for the Pokitto wins Simulator or for the Pokitto HW

- #define POKITTO_USE_REPL_ONLY
  - This tells if we intend to use REPL only.
  
In makefile:

- CROSS_COMPILE = /cygdrive/P/bin/arm-none-eabi-
  - This is the path to the armgcc compiler, which comes with Embitz
  - The drive P: is substed like this: 
    - subst P: "c:\Program Files (x86)\EmBitz\1.11\share\em_armgcc"  
  
- POKITTO_DIR = ../../../PokittoLib2/Pokitto/POKITTO_LIBS/MicroPython
  - This is the path to the MicroPython folder under the PokittoLib

In copy_lib_and_qstr.sh:

- ../../../PokittoLib2/Pokitto/POKITTO_LIBS/MicroPython
  - This is the path to the MicroPython folder under the PokittoLib

FAQ
-----------------------------
Q: How to set source file path for EmBitz GDB debugger for MicroPython library sources?
A: Menu: "Debug/Interfaces/GDB Additionals/. Add to "Before Connect textbox" : "dir ../micropython2/ports/pokitto/modules"



