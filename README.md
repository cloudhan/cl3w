# cl3w: Simple OpenCL library loading

## Introduction

Inspired by [gl3w](https://github.com/skaslev/gl3w), cl3w is the easiest way to get your hands on the functionality offered by the OpenCL on **various platforms**. E.g. on Android, OpenCL dynamic library generally does not sit on standard location and can vary from vendor to vendor, so you won't want the dynamic linker do the loading for you.

Its main part is a simple [cl3w_gen.py](./cl3w_gen.py) python script that downloads the Khronos [cl.xml](https://github.com/KhronosGroup/OpenCL-Docs/blob/main/xml/cl.xml) and generates cl3w.h and cl3w.c from it. Those files can then be added and linked into your project.

See [vector_add.cpp](test/vector_add.cpp) for an functional example.

## Example

As of writing, the `cl.xml` from https://raw.githubusercontent.com/KhronosGroup/OpenCL-Docs/v3.0.10/xml/cl.xml is tested.

```bash
python ./cl3w_gen.py --cl_std=1.2 --cl_xml=https://raw.githubusercontent.com/KhronosGroup/OpenCL-Docs/v3.0.10/xml/cl.xml
cmake -S. -Bbuild -GNinja
cmake --build build
./build/test/vector_add
```

If you `ldd` on *nix or `dumpbin.exe /DEPENDENTS` on Windows, OpenCL library will not show in the list. If you don't
understand what this means, then you probably don't need cl3w.

## Documentation

No docs at the moment.

Public interfaces::

- `cl3wInit()`: Load OpenCL library from builtin library search list at runtime and initialize all OpenCL APIs.
- `cl3wInit2(...)`: Load OpenCL library form user provided pathes and initialize all OpenCL APIs.
- `cl3wUnload()`

User facing macros:

- `CL3W_NO_CL_API_DEFINES`: If not defined, OpenCL APIs names are preprocessed to be dynamically loaded function pointers. Save one round trip of function call indirection.
