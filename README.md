# cmake_test
--

## Requirements
To compile this project severall libraries are needed. To obtain them vcpkg is used. 
To install vcpkg look [here](https://github.com/microsoft/vcpkg). Attention the default target in vcpkg is 32 bit.

* Install getopt with vcpkg. `vcpkg install getopt`. 
* Install boost with vcpkg. `vcpkg install boost`. 
* Add to cmake configuration: `-DCMAKE_TOOLCHAIN_FILE=c:\\dev\\vcpkg\\scripts\\buildsystems\\vcpkg.cmake`
