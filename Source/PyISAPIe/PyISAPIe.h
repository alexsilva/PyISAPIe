// https://pyisapie.svn.sourceforge.net/svnroot/pyisapie/Tags/1.1.0-rc1/PyISAPIe/PyISAPIe.h
// 151 2009-05-20 20:02:46 -0700 (Wed, 20 May 2009)
// (C)2008 Phillip Sitbon <phillip@sitbon.net>
//
// Global include file.
//
// This file is *almost* API-ready. The only things that need
// to be changed are the inclusion of Trace.h and exporting/
// importing of the PyContext_* functions, instead of having
// macros.
//
#ifndef _PyISAPIe_
#define _PyISAPIe_

// Windows Server 2003: 0x0502
// Windows XP:          0x0501
// Windows 2000:        0x0500
//
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0502
#endif

// The windows ISAPI API header
//
#define WIN32_LEAN_AND_MEAN

#include <httpext.h>

// Defined again in <structmember.h>
#undef WRITE_RESTRICTED

// Main Python include file
extern "C" {
#include <Python.h>
#include <structmember.h>
#include <frameobject.h>
}

// Remove the new CRT deprecation warnings
//
// Visual C++ 2005:    1400
//
#if _MSC_VER >= 1400
#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif
#pragma warning(disable:4996)
#endif

// Uncomment the lines below to enable traces.
// use NO_SEH to disable structured exception handling.
//

// #define TRACE_LOG  "C:\\Trace\\PyISAPIe-Trace"
// #define NO_SEH

#include "Trace.h"
//
// -

// Package that the modules are inside of.
//
#define HTTP_PKG    "Http"

// Package module & function that dispatch
// requests, and the global variable holding
// the name of the current interpreter.
//
#define DISP_MOD    "Isapi"
#define DISP_FN     "Request"
#define INTERP_VAR  "__interp__"

// Module that config info is retrieved from
//
#define CFG_MOD     "Config"

// Helper macros
//
#define _DOT(x, y)   x "." y
#define _PKG(x)     _DOT(HTTP_PKG,x)

// Version: [1][1] = [major][minor]
//
#define VER_NUM   0x0101
#define VER_STR   "1.1.0-rc1"

// Forward definition
struct Interpreter;

// For PyISAPIe functions, to allow for
// different calling conventions
//
#define PyISAPIe_Func(Rtype)  Rtype __fastcall

// Current request context
//
struct Context {
    LPEXTENSION_CONTROL_BLOCK ECB;
    Interpreter *Interp;
    unsigned char Flags;

    HSE_SEND_HEADER_EX_INFO Headers;
    PyObject *HeaderList;
    bool HaveGIL;

    int ContentLength;
    char *RealBuffer;
    char *Buffer;
    unsigned long RealBufferSize;
    unsigned long BufferUsed;
};

// Struct stored in dictionary
struct Interpreter {
    PyInterpreterState *InterpreterState;
    PyObject *Name;               /* string     */
    PyObject *RequestFunction;    /* function   */
    ternaryfunc RequestFunctionPtr; /* C function */
    bool Initialized;

    // Config parameters
    unsigned long BufferSize;
    bool KeepAlive;
    char *DefaultHeaders;
    unsigned long DefaultHeaderLength;

    void *Info;               /* user data  */
};

// Default interval to switch threads
//
#define DEFAULT_CHECK_INTERVAL    1024//2147483647

// Min/max configurable buffer size
//
#define MIN_BUFFER_SIZE           64
#define MAX_BUFFER_SIZE           131072

// For chunked transfer encoding
//

#define CHUNK_HDR       "          \r\n"
#define CHUNK_FTR       "\r\n"
#define CHUNK_FIN       "0\r\n\r\n"

#define CHUNK_HDR_LEN   12 // "XxXxXxXxXx\r\n"
#define CHUNK_FTR_LEN   2
#define CHUNK_FIN_LEN   5


// Context Flags
//
#define CTX_READY             0x01  // Initialized
#define CTX_DATA_SENT         0x02  // Data sent
#define CTX_HDRS_SENT         0x04  // Headers sent
#define CTX_KEEPALIVE         0x08  // Keepalive enabled
#define CTX_HTTP11            0x10  // HTTP/1.1 enabled
#define CTX_CHUNKED           0x20  // Chunked transfer (1.1 && Keepalive)
#define CTX_USER_FLAGS        0xD0  // Unused bits

// Helper functions/macros:
//

#define USING_CHUNKED(x)      (x & CTX_CHUNKED)

// Context-related
//

// Defined in Main.cpp
extern DWORD TlsIndex_Ctx;

// for ERR_RET - for functions w/o returns.
#ifndef NONE
#define NONE
#endif

#define PyContext_Set(CTX)      (TlsSetValue(TlsIndex_Ctx,(LPVOID)CTX)!=0)
#define PyContext_Load(CTX)     Context *CTX((Context *)TlsGetValue(TlsIndex_Ctx));
#define PyContext_IsValid(CTX)  ( (CTX) != (Context *)0 )
#define PyContext_IsReady(CTX)  (PyContext_IsValid(CTX) && ((CTX)->Flags & CTX_READY))

#define PyContext_Fail(ERR_RET) \
  { \
    PyErr_SetString(PyExc_Exception,  \
      "Not currently processing a request"); \
    return ERR_RET; \
  }

#define PyContext_Check(CTX, ERR_RET) \
  if ( !PyContext_IsReady(CTX) ) PyContext_Fail(ERR_RET)

#define PyContext_LoadAndCheck(CTX, ERR_RET)  \
  PyContext_Load(CTX) \
  PyContext_Check(CTX,ERR_RET)

// TODO: Add debug versions with asserts against HaveGIL

#define PyContext_Unblock(CTX) \
  ( (CTX)->HaveGIL ? NULL : ((CTX)->HaveGIL = false, PyEval_SaveThread()) )

#define PyContext_Block(CTX, TS) \
  if (!(CTX)->HaveGIL && (TS != NULL)) (CTX)->HaveGIL = true, PyEval_RestoreThread(TS);

#define PyContext_Async_Begin(CTX) \
  { PyThreadState *const Ts_Save_ = PyContext_Unblock(CTX);

#define PyContext_Async_End(CTX) \
  PyContext_Block(CTX,Ts_Save_); }


#endif // !_PyISAPIe_
