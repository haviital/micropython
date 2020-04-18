This is Windows Pokitto Simulator port of MicroPython.


Building under Cygwin
---------------------

Install following packages using cygwin's setup.exe:

* mingw64-i686-gcc-core
* mingw64-x86_64-gcc-core
* make

Build using (64-bit target library binary):

    make CROSS_COMPILE=x86_64-w64-mingw32- lib
    
Or (verbose and debug) use:
    make CROSS_COMPILE=x86_64-w64-mingw32- V=1 DEBUG=1 lib

To clean up use:
     make CROSS_COMPILE=x86_64-w64-mingw32- clean
   

Building mpy-cross (if needed)
------------------------------
cd ../../mpy-cross
make CROSS_COMPILE=x86_64-w64-mingw32-

FAQ
-----------------------------
Q: How to set source file path for CodeBlocks GDB debugger for MicroPython library sources?
A: Menu: "Project/Properties/Debugger/. Add to "Additional Debugger Search Dirs" textbot  e.g. a path "C:\git\micropython2\ports\pokitto\modules" (absolute path)
