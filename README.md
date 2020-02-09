Pagedon't
=========

A quick hack to forward Pageant requests to OpenSSH for Windows.

Building (on Linux)
-------------------
Depends on CMake and Mingw-w64. Probably builds as-is on MSYS2.
```
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain-mingw-w64.cmake
make
```
