# cl3w: Simple OpenCL library loading

## Introduction

Inspired by [gl3w](https://github.com/skaslev/gl3w), cl3w is the easiest way to get your hands on the functionality offered by the OpenCL on **various platforms**. E.g. on Android, OpenCL dynamic library generally does not sit on standard location and can vary from vendor to vendor, so you won't want the dynamic linker do the loading for you.

Its main part is a simple [cl3w_gen.py](./cl3w_gen.py) python script that downloads the Khronos [cl.xml](https://github.com/KhronosGroup/OpenCL-Docs/blob/main/xml/cl.xml) and generates cl3w.h and cl3w.c from it. Those files can then be added and linked into your project.

See [vector_add.cpp](test/vector_add.cpp) for an functional example.
