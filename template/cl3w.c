#define CL3W_NO_CL_API_DEFINES
#include "cl3w.h"

#include <stdio.h>

#define CL3W_RET_IF_OK(expr) if ((expr) == CL3W_OK) return CL3W_OK
#define CL3W_RET_IF_ERROR(expr) do {       \
    CL3W_STATUS status = (expr);           \
    if (status != CL3W_OK) return status;  \
} while(0)

static const char* get_probe_api_name(void);
static CL3WclAPI get_api(const char *api_name);
static CL3W_STATUS load_libcl(void);
static void load_apis(void);
static void unload_apis(void);

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>

static const char* default_lib_paths[] = {
  "OpenCL.dll"
};

static HMODULE libcl;

static CL3W_STATUS open_lib(const char* path, HMODULE* out_lib) {
    HMODULE lib = LoadLibraryA(path);
    if (!lib) {
        return CL3W_ERROR_LIBRARY_OPEN;
    }

    if (!GetProcAddress(lib, get_probe_api_name())) {
        FreeLibrary(lib);
        return CL3W_ERROR_LIBRARY_OPEN;
    }

    *out_lib = lib;
    return CL3W_OK;
}


static CL3W_STATUS open_libcl(const char** libpaths, size_t npaths) {
    if (libpaths && npaths) {
        int i;
        for (i = 0; i < npaths; i++) {
            CL3W_RET_IF_OK(open_lib(libpaths[i], &libcl));
        }
    }
    libcl = (PVOID)0;
    return CL3W_ERROR_LIBRARY_OPEN;
}

static void close_libcl(void) {
    FreeLibrary(libcl);
}

static CL3WclAPI get_api(const char *api_name) {
    CL3WclAPI res;
    res = (CL3WclAPI)GetProcAddress(libcl, api_name);
    return res;
}
#else // POSIX
#include <dlfcn.h>

#if defined(__ANDROID__)
static const char *default_lib_paths[] = {
  "/system/lib64/libOpenCL.so",
  "/system/vendor/lib64/libOpenCL.so",
  "/system/vendor/lib64/egl/libGLES_mali.so",
  "/system/vendor/lib64/libPVROCL.so",
  "/data/data/org.pocl.libs/files/lib64/libpocl.so",
  "/system/lib/libOpenCL.so",
  "/system/vendor/lib/libOpenCL.so",
  "/system/vendor/lib/egl/libGLES_mali.so",
  "/system/vendor/lib/libPVROCL.so",
  "/data/data/org.pocl.libs/files/lib/libpocl.so",
  "libOpenCL.so"
};
#elif defined(__linux__)
static const char *default_lib_paths[] = {
  "/usr/lib/libOpenCL.so",
  "/usr/local/lib/libOpenCL.so",
  "/usr/local/lib/libpocl.so",
  "/usr/lib64/libOpenCL.so",
  "/usr/lib32/libOpenCL.so",
  "libOpenCL.so"
};
#elif defined(__APPLE__) || defined(__MACOSX)
static const char *default_lib_paths[] = {
  "/System/Library/Frameworks/OpenCL.framework/OpenCL"
  "libOpenCL.so",
};
#endif

static void* libcl;

static CL3W_STATUS open_lib(const char* path, void** out_lib) {
    void* lib = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
    if (!lib) {
        return CL3W_ERROR_LIBRARY_OPEN;
    }

    if (!dlsym(lib, get_probe_api_name())) {
        dlclose(lib);
        return CL3W_ERROR_LIBRARY_OPEN;
    }

    *out_lib = lib;
    return CL3W_OK;
}

static CL3W_STATUS open_libcl(const char** libpaths, size_t npaths) {
    if (libpaths && npaths) {
        int i;
        for (i = 0; i < npaths; i++) {
            CL3W_RET_IF_OK(open_lib(libpaths[i], &libcl));
        }
    }
    libcl = NULL;
    return CL3W_ERROR_LIBRARY_OPEN;
}

static void close_libcl(void)
{
    if (libcl) {
        dlclose(libcl);
        libcl = NULL;
    }
}

static CL3WclAPI get_api(const char *api_name) {
    CL3WclAPI res;
    res = (CL3WclAPI)dlsym(libcl, api_name);
    return res;
}
#endif

/* generated cl3w_api_names */

/* generated stub dummies */

CL3W_API union CL3WAPIs cl3w_apis;

/* generated stub impls */


#define CL3W_ARRAY_SIZE(x)  (sizeof(x) / sizeof((x)[0]))

static void load_apis(void) {
    unload_apis();
    size_t i;
    for (i = 0; i < CL3W_ARRAY_SIZE(cl3w_api_names); i++) {
        CL3WclAPI func = get_api(cl3w_api_names[i]);
        if (func) {
            // printf("%s loaded\n", cl3w_api_names[i]);
            cl3w_apis.ptr[i] = func;
        }
    }
}

/* generated unload_apis */

/* generated get_probe_api_name */

CL3W_STATUS cl3wInit(void) {
    return cl3wInit2(default_lib_paths, CL3W_ARRAY_SIZE(default_lib_paths));
}

CL3W_STATUS cl3wInit2(const char** libpaths, size_t npaths) {
    CL3W_RET_IF_ERROR(open_libcl(libpaths, npaths));
    load_apis();
    return CL3W_OK;
}

CL3W_STATUS cl3wUnload() {
    unload_apis();
    close_libcl();
    return CL3W_OK;
}

#undef CL3W_ARRAY_SIZE
