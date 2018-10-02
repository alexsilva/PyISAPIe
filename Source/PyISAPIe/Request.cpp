// https://pyisapie.svn.sourceforge.net/svnroot/pyisapie/Tags/1.1.0-rc1/PyISAPIe/Request.cpp
// 151 2009-05-20 20:02:46 -0700 (Wed, 20 May 2009)
// (C)2008 Phillip Sitbon <phillip@sitbon.net>
//
// Request handling related functions.
//
#include "Request.h"
#include "Interpreter.h"
#include "Module/Module.h"
#include "ReadWrite.h"
#include "Header.h"

// For alloca()
#include <malloc.h>

// The example in the link below says:
//   "The following define is needed if httpext.h is pre-Windows:"
//   lol.
//
#ifndef   HSE_REQ_SET_FLUSH_FLAG
#define   HSE_REQ_SET_FLUSH_FLAG  (HSE_REQ_END_RESERVED+43)
#endif

/////////////////////////////////////////////////////////////////////
//  HandleRequest()
//
PyISAPIe_Func(DWORD) HandleRequest(Context &Ctx) {
    SAFE_BEGIN

    static PyObject *const EmptyTuple = PyTuple_New(0);

    // Sorry IIS7, I want deterministic behavior.
    // Disable IIS buffering and use our own scheme.
    // This would be in ISAPI.cpp, but it MIGHT be configurable
    //   in the future - no promises.
    // Not sure if SetLastError(0) is necessary in IIS 5/6.
    Ctx.ECB->ServerSupportFunction(
            Ctx.ECB->ConnID, HSE_REQ_SET_FLUSH_FLAG, (LPVOID) TRUE, NULL, NULL
    );
    // Related links:
    // http://support.microsoft.com/kb/946086
    // http://forums.iis.net/t/1151673.aspx
    //


    if (!Ctx.Interp->Initialized)
        return Fail(Ctx, "Could not initialize interpreter");


    //////////////////////////////////////////////////////////
    // Set up parameters for the keeplaive mechanism
    //

    // Doesn't bother checking if the connection
    // will be closed.
    //
    if (Ctx.Interp->KeepAlive) {
        // Determine if we're using HTTP/1.1
        //

        char Ver[16];
        DWORD Info = 15;

        if (!Ctx.ECB->GetServerVariable(Ctx.ECB->ConnID, "SERVER_PROTOCOL",
                                        Ver, &Info)) {
            const DWORD Err(GetLastError());
            PyErr_SetFromWindowsErr(Err);
            return Fail(Ctx, "Could not determine protocol version");
        }

        if (!strcmp(Ver, "HTTP/1.1"))
            Ctx.Flags |= CTX_HTTP11;

        // Determine keepalive status
        //

        BOOL KeepAlive = FALSE;

        if (!Ctx.ECB->ServerSupportFunction(Ctx.ECB->ConnID,
                                            HSE_REQ_IS_KEEP_CONN, &KeepAlive, NULL, NULL)) {
            const DWORD Err(GetLastError());
            PyErr_SetFromWindowsErr(Err);
            return Fail(Ctx, "Could not determine keepalive status");
        }

        // Note: This is set to TRUE correctly in IIS 6; however,
        // IIS 5.1 (for me) returns -1 instead of TRUE when keepalive
        // is enabled. Furthermore, regardless of configuration,
        // this will always be FALSE for versions < 6.0 running out
        // of process!

        if (KeepAlive /*== TRUE*/ ) {
            Ctx.Flags |= CTX_KEEPALIVE;
            Ctx.Headers.fKeepConn = TRUE;

            // Assign a single flag
            if (Ctx.Flags & CTX_HTTP11)
                Ctx.Flags |= CTX_CHUNKED;
        }
    }

    //////////////////////////////////////////////////////////
    // Initialize buffer
    //
    if (Ctx.Interp->BufferSize) {
        if (USING_CHUNKED(Ctx.Flags)) {
            // TODO: Consolidate all teh chunked transfer encoding stuff.
            //
            Ctx.RealBufferSize = Ctx.Interp->BufferSize +
                                 CHUNK_HDR_LEN + CHUNK_FTR_LEN;
            Ctx.RealBuffer = (char *) alloca(Ctx.RealBufferSize + CHUNK_FIN_LEN);
            Ctx.Buffer = Ctx.RealBuffer + CHUNK_HDR_LEN;
            memcpy(Ctx.RealBuffer, CHUNK_HDR, CHUNK_HDR_LEN);
            memcpy(&Ctx.RealBuffer[Ctx.RealBufferSize - CHUNK_FTR_LEN],
                   CHUNK_FTR, CHUNK_FTR_LEN);
            ltoa(Ctx.Interp->BufferSize, Ctx.RealBuffer, 16);
            Ctx.RealBuffer[strlen(Ctx.RealBuffer)] = ';';
        } else {
            Ctx.RealBufferSize = Ctx.Interp->BufferSize;
            Ctx.Buffer = Ctx.RealBuffer = (char *) alloca(Ctx.RealBufferSize);
        }
    }

    //////////////////////////////////////////////////////////
    // Use default headers
    //
    Ctx.Headers.pszHeader = Ctx.Interp->DefaultHeaders;
    Ctx.Headers.cchHeader = Ctx.Interp->DefaultHeaderLength;

    //////////////////////////////////////////////////////////
    // Complete init AFTER buffer is ready
    //
    Ctx.Flags |= CTX_READY;

    //////////////////////////////////////////////////////////
    // Dispatch the request handler
    //

    //PyThreadState_Swap(ThreadState);
    //PyEval_AcquireLock();

    ModuleBeginRequest(Ctx);

    Trace(TRP"Calling dispatch function (0x%X)", Ctx.Interp->RequestFunctionPtr);

    // Call the request function - adapted from Objects\abstract.c
    //
    /*
    PyObject *const
      Result(
        (* Ctx.Interp->RequestFunctionPtr)
          (Ctx.Interp->RequestFunction, EmptyTuple, NULL)
      );
    */
    PyObject *const Result = PyObject_Call(Ctx.Interp->RequestFunction, EmptyTuple, NULL);
    //
    //

    // Treat SystemExit as a "non-error" exception.
    if (!Result) {
        // Use !PyErr_Occurred() as a short-circuit.
        // Doesn't care about a NULL result without the error being set.
        if (!PyErr_Occurred() || !PyErr_ExceptionMatches(PyExc_SystemExit))
            return Fail(Ctx, "Request handler failed");
    } else
        Py_DECREF(Result);

    Trace(TRP"Dispatch function completed");

    if (WriteFinishNeeded(Ctx)) {
        // Finish writing (note: SendHeaders, if called, grabs the GIL again)
        PyContext_Async_Begin(&Ctx);
            WriteFinish(Ctx);
        PyContext_Async_End(&Ctx);
    }

    return HSE_STATUS_SUCCESS;

    SAFE_END
    return HSE_STATUS_ERROR;
}
