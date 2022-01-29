#ifndef __gl3w_h_
#define __gl3w_h_

#include <CL/opencl.h>

#ifndef CL3W_API
#define CL3W_API
#endif

#if defined(_WIN32)
#ifndef CL_API_ENTRY
#define CL_API_ENTRY
#endif
#ifndef CL_API_ENTRYP
#define CL_API_ENTRYP CL_API_ENTRY*
#endif
#ifndef CL_API_CALL
#define CL_API_CALL     __stdcall
#endif
#ifndef CL_CALLBACK
#define CL_CALLBACK     __stdcall
#endif
#else
#ifndef CL_API_ENTRY
#define CL_API_ENTRY
#endif
#ifndef CL_API_ENTRYP
#define CL_API_ENTRYP CL_API_ENTRY*
#endif
#ifndef CL_API_CALL
#define CL_API_CALL
#endif
#ifndef CL_CALLBACK
#define CL_CALLBACK
#endif
#endif

#ifndef CLAPI
#define CLAPI
#endif
#ifndef CLAPIP
#define CLAPIP CLAPI *
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define CL3W_STATUS int
#define CL3W_OK (CL3W_STATUS)0
#define CL3W_ERROR_INIT (CL3W_STATUS)-1
#define CL3W_ERROR_LIBRARY_OPEN (CL3W_STATUS)-2

typedef void (*CL3WclAPI)(void);

/// Init with default path
CL3W_API CL3W_STATUS cl3wInit();

/// Init with user defined heuristics
CL3W_API CL3W_STATUS cl3wInit2(const char** libpaths, size_t npaths);

/// Unload OpenCL()
CL3W_API CL3W_STATUS cl3wUnload();

/* generated typedefs */

/* generated CL3WAPIs */

CL3W_API extern union CL3WAPIs cl3w_apis;

#ifndef CL3W_NO_CL_API_DEFINES
/* generated defines */
#endif

#ifdef __cplusplus
}
#endif
#endif
