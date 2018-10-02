// https://pyisapie.svn.sourceforge.net/svnroot/pyisapie/Tags/1.1.0-rc1/PyISAPIe/ISAPI.cpp
// 151 2009-05-20 20:02:46 -0700 (Wed, 20 May 2009)
// (C)2008 Phillip Sitbon <phillip@sitbon.net>
//
// ISAPI module initialization & implementation.
//
#include "PyISAPIe.h"
#include "Interpreter.h"
#include "Request.h"
#include "ReadWrite.h"

// For alloca()
#include <malloc.h>

CRITICAL_SECTION CsReq;

/*
 * GetExtensionVersion()
 */
BOOL WINAPI GetExtensionVersion(HSE_VERSION_INFO *pVer) {
    pVer->dwExtensionVersion = HSE_VERSION;
    strncpy_s(pVer->lpszExtensionDesc, VER_STR " [" __TIMESTAMP__ "]", HSE_MAX_EXT_DLL_NAME_LEN);
    return TRUE;
}

/*
 * HttpExtensionProc()
 */
DWORD WINAPI HttpExtensionProc(LPEXTENSION_CONTROL_BLOCK lpECB) {

    Context Ctx = {
        lpECB,
        NULL,
        0,
        {"200 OK", NULL, 6, 0, FALSE},
        NULL,
        false,
        -1,
        NULL,
        NULL,
        0,
        0
    };

    Trace(TRP "Handling request");

    EnterCriticalSection(&CsReq);

    Ctx.Interp = AcquireInterpreter(Ctx);

    if (!Ctx.Interp) {
        PyEval_ReleaseLock();
        LeaveCriticalSection(&CsReq);
        return Fail(Ctx, "Could not acquire interpreter");
    }

    Trace(TRP"Interpreter acquired (0x%08X %s)", Ctx.Interp->InterpreterState, PyString_AS_STRING(Ctx.Interp->Name));

    lpECB->dwHttpStatusCode = 200;

    if (!PyContext_Set(&Ctx)) {
        ReleaseInterpreter();
        Ctx.Interp = NULL;
        LeaveCriticalSection(&CsReq);
        return Fail(Ctx, "Could not set context");
    }

    LeaveCriticalSection(&CsReq);
    const DWORD Ret(HandleRequest(Ctx));

    ReleaseInterpreter();
    return Ret;
}
