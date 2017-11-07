This is Windows Pokitto Simulator port of MicroPython.


Building under Cygwin
---------------------

Install following packages using cygwin's setup.exe:

* mingw64-i686-gcc-core
* mingw64-x86_64-gcc-core
* make

Build using:

    make CROSS_COMPILE=i686-w64-mingw32- lib
    

Building mpy-cross (if needed)
------------------------------
cd ../../mpy-cross
make CROSS_COMPILE=i686-w64-mingw32-

