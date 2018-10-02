// https://pyisapie.svn.sourceforge.net/svnroot/pyisapie/Tags/1.1.0-rc1/PyISAPIe/ReadWrite.cpp
// 151 2009-05-20 20:02:46 -0700 (Wed, 20 May 2009)
// (C)2008 Phillip Sitbon <phillip@sitbon.net>
//
// Reading & writing utility functions.
//
#include "ReadWrite.h"
#include "Header.h"

#include <stdarg.h>
#include <malloc.h>
#include <direct.h>
#include "PyISAPIe.h"

static PyISAPIe_Func(bool) WriteChunk(Context &Ctx, DWORD Length, const char *const Data);

/////////////////////////////////////////////////////////////////////
//  Write()
//
//  Does the actual sending of data.
//

PyISAPIe_Func(bool) Write(Context &Ctx, DWORD Length,
                          const char *Data) {
    DWORD Ret;

    while (Length) {
        Ret = Length;

        if (!Ctx.ECB->WriteClient(Ctx.ECB->ConnID, (LPVOID) Data, &Ret, HSE_IO_SYNC)) {
            // note: data may have been sent even if the flag isn't set.
            Trace(TRP"Write of %u bytes failed (0x%08X)", Length, GetLastError());
            return false;
        }

        Length -= Ret;

#ifdef TRACE_LOG
        if ( Length )
          Trace(TRP"Wrote %u bytes (%u remaining)", Ret, Length);
        else
          Trace(TRP"Wrote %u bytes", Ret);
#endif

        Data += Ret;
    }

    Ctx.Flags |= CTX_DATA_SENT;
    return true;

    //Ctx.ECB->WriteClient(Ctx.ECB->ConnID, (LPVOID)Data,
    //  &Length, HSE_IO_SYNC);

    //Trace(TRP"Wrote %u bytes", Length);

    //Ctx.Flags |= CTX_DATA_SENT;
}

/////////////////////////////////////////////////////////////////////
//  SendBuffer()
//
//  Sends the current buffer contents.
//
static PyISAPIe_Func(bool) SendBuffer(Context &Ctx, const bool &Last = false) {
    if (!(Ctx.Flags & CTX_HDRS_SENT)) {
        if (Last) {
            Ctx.ContentLength = Ctx.BufferUsed;
            if (!SendHeaders(Ctx))
                return false;
            if (Ctx.BufferUsed)
                return Write(Ctx, Ctx.BufferUsed, Ctx.Buffer);
            return true;
        }

        if (!SendHeaders(Ctx))
            return false;
    } else if (Last) {
        // TODO: see if two writes are faster

        if (Ctx.BufferUsed) {
            if (USING_CHUNKED(Ctx.Flags)) {
                memset(Ctx.RealBuffer, ' ', 10);
                ltoa(Ctx.BufferUsed, Ctx.RealBuffer, 16);
                // replace the \0 with ';'
                Ctx.RealBuffer[strlen(Ctx.RealBuffer)] = ';';
                memcpy(&Ctx.RealBuffer[CHUNK_HDR_LEN + Ctx.BufferUsed],
                       CHUNK_FTR CHUNK_FIN, CHUNK_FTR_LEN + CHUNK_FIN_LEN);
                Ctx.BufferUsed += CHUNK_HDR_LEN + CHUNK_FTR_LEN + CHUNK_FIN_LEN;

                return Write(Ctx, Ctx.BufferUsed, Ctx.RealBuffer);
            }

            return Write(Ctx, Ctx.BufferUsed, Ctx.Buffer);
        }

        if (USING_CHUNKED(Ctx.Flags))
            return Write(Ctx, CHUNK_FIN_LEN, CHUNK_FIN);

        return true;
    }

    return Write(Ctx, Ctx.RealBufferSize, Ctx.RealBuffer);
}

/////////////////////////////////////////////////////////////////////
//  WriteNeedsFinish()
//
PyISAPIe_Func(bool) WriteFinishNeeded(Context &Ctx) {
    // Let the caller know if releasing the GIL is necessary
    //

    if (Ctx.RealBufferSize)
        return true;

    if (!(Ctx.Flags & CTX_HDRS_SENT))
        return true;

    if (USING_CHUNKED(Ctx.Flags))
        return true;

    return false;
}

/////////////////////////////////////////////////////////////////////
//  WriteFinish()
//
PyISAPIe_Func(bool) WriteFinish(Context &Ctx) {
    if (Ctx.RealBufferSize)
        return SendBuffer(Ctx, true);

    if (!(Ctx.Flags & CTX_HDRS_SENT)) {
        // This is the final send:
        // stops transfer-encoding header from being sent
        // (if chunked)
        Ctx.ContentLength = 0;
        return SendHeaders(Ctx);
    }

    if (USING_CHUNKED(Ctx.Flags))
        return Write(Ctx, CHUNK_FIN_LEN, CHUNK_FIN);

    return true;
}

/////////////////////////////////////////////////////////////////////
//  WriteClient(Data)
//
PyISAPIe_Func(bool) WriteClient(Context &Ctx, DWORD Length,
                                const char *Data) {
    if (Ctx.RealBufferSize) {
        const unsigned long BufferSize = Ctx.Interp->BufferSize;
        const unsigned long FillSize = BufferSize - Ctx.BufferUsed;

        // Complete buffer before send
        if (Ctx.BufferUsed && (Length > FillSize)) {
            memcpy(&Ctx.Buffer[Ctx.BufferUsed], Data, FillSize);
            Length -= FillSize;
            Data += FillSize;
            Ctx.BufferUsed = 0;
            if (!SendBuffer(Ctx))
                return false;
        }

        // Send full buffer - still need to copy it over
        while (Length > BufferSize) {
            memcpy(Ctx.Buffer, Data, BufferSize);
            if (!SendBuffer(Ctx))
                return false;
            Length -= BufferSize;
            Data += BufferSize;
        }

        // Buffer only
        if (Length) {
            memcpy(&Ctx.Buffer[Ctx.BufferUsed], Data, Length);
            Ctx.BufferUsed += Length;
        }

        return true;
    } else if (USING_CHUNKED(Ctx.Flags)) {
        if (!(Ctx.Flags & CTX_HDRS_SENT))
            if (!SendHeaders(Ctx)) return false;

        // Save the stack
        while (Length > MAX_BUFFER_SIZE) {
            if (!WriteChunk(Ctx, MAX_BUFFER_SIZE, Data))
                return false;
            Length -= MAX_BUFFER_SIZE;
            Data += MAX_BUFFER_SIZE;
        }

        if (Length)
            if (!WriteChunk(Ctx, Length, Data)) return false;

        return true;
    }

    if (!(Ctx.Flags & CTX_HDRS_SENT))
        if (!SendHeaders(Ctx)) return false;

    return Write(Ctx, Length, Data);
}

/////////////////////////////////////////////////////////////////////
//  WriteChunk()
//
PyISAPIe_Func(bool) WriteChunk(Context &Ctx, DWORD Length,
                               const char *const Data) {
    char *const All = (char *) alloca(Length + CHUNK_HDR_LEN + CHUNK_FTR_LEN);
    memcpy(All, CHUNK_HDR, CHUNK_HDR_LEN);
    memcpy(&All[CHUNK_HDR_LEN], Data, Length);
    ltoa(Length, All, 16);
    All[strlen(All)] = ';';
    Length += CHUNK_HDR_LEN;
    memcpy(&All[Length], CHUNK_FTR, CHUNK_FTR_LEN);
    Length += CHUNK_FTR_LEN;
    return Write(Ctx, Length, All);
}

/////////////////////////////////////////////////////////////////////
//  WriteClient(Formatted string)
//
PyISAPIe_Func(bool) WriteClient(Context &Ctx, const char *const Format, ...) {
    va_list Args;
            va_start(Args, Format);
    const int Length = _vscprintf(Format, Args);
    if (Length < 0) return true; // error?
    char *const Out = (char *) alloca(Length + 1);
    Out[Length] = 0;
    vsprintf(Out, Format, Args);
            va_end(Args);
    // alloca + tail-call == bad?
    return WriteClient(Ctx, (DWORD) Length, Out);
}

/////////////////////////////////////////////////////////////////////
//  ReadClient()
//
PyISAPIe_Func(DWORD) ReadClient(Context &Ctx, DWORD Length, void *const Data) {
    if (!Length)
        Length = Ctx.ECB->cbTotalBytes;

    if (!Data)
        // Return the size of the the data that would be read
        return min(Length, Ctx.ECB->cbTotalBytes);

    DWORD Ret, Total = 0;

    if (Length > Ctx.ECB->cbAvailable) {
        memcpy(Data, Ctx.ECB->lpbData, Ctx.ECB->cbAvailable);
        Length -= Ctx.ECB->cbAvailable;
        Total += Ctx.ECB->cbAvailable;
        Ctx.ECB->cbTotalBytes -= Ctx.ECB->cbAvailable;
        Ctx.ECB->cbAvailable = 0;
    } else {
        memcpy(Data, Ctx.ECB->lpbData, Length);
        Ctx.ECB->cbTotalBytes -= Length;
        Ctx.ECB->cbAvailable -= Length;
        return Length;
    }

    if (Length > Ctx.ECB->cbTotalBytes)
        Length = Ctx.ECB->cbTotalBytes;

    while (Ret = Length) {
        if (!Ctx.ECB->ReadClient(Ctx.ECB->ConnID,
                                 &((char *) Data)[Total], &Ret)) {
            // what about Ctx.ECB->cbTotalBytes here?
            return Total;
        }

        Total += Ret;
        Length -= Ret;
        Ctx.ECB->cbTotalBytes -= Ret;
    }

    return Total;
}

/////////////////////////////////////////////////////////////////////
//  Fail()
//
PyISAPIe_Func(DWORD) Fail(Context &Ctx, const char *const Format, ...) {
    const static char ErrorHeader[] =
            //"<html>\r\n"
            "<head><title>Internal Server Error</title></head>\r\n"
            "<body font=\"Verdana\" style=\"font-family: sans-serif\">\r\n"
            "<font size=\"+3\"><b>Internal Server Error</b></font><br>\r\n"
            "An error occurred processing this request.<br>\r\n"
            "<hr width=\"100%\" size=\"1\" style=\"margin:0;\" color=\"black\" noshade>\r\n";

    const static char Part1[] = "<pre><font color=blue>";
    const static char Part2[] = "</font>\r\n<font color=red>\r\n";
    const static char Part3[] = "\r\n</font>\r\n</pre>";
    const static char APart1[] =
            "<font size=\"+1\">Additional Information</font><br>\r\n"
            "\r\n<hr width=\"100%\" size=\"1\" style=\"margin:0;\" color=\"black\" noshade>\r\n"
            "<pre><font color=blue>";
    const static char APart2[] = "\r\n</font></pre>";

    static PyObject *const ErrorHeaders =
            PyString_FromString("Content-Type: text/html");

    int ErrorStringLength = 0;
    char *ErrorString;
    PyObject *Err;

    if (Format) {
        va_list Args;
                va_start(Args, Format);
        ErrorStringLength = _vscprintf(Format, Args);
        ErrorString = (char *) alloca(ErrorStringLength + 1);
        ErrorString[ErrorStringLength] = 0;
        vsprintf(ErrorString, Format, Args);
                va_end(Args);
    } else {
        ErrorStringLength = 0;
        ErrorString = (char *) alloca(1);
        ErrorString[0] = 0;
    }

    // Make sure printing from PyErr_Print() works
    Ctx.Flags |= CTX_READY;

    // Extra trace info
#ifdef TRACE_LOG

    if ( Ctx.Interp && (Err = PyErr_Occurred()) )
    {
      PyObject *Exc, *Val, *Tb, *Str;
      PyErr_Fetch(&Exc, &Val, &Tb);
      PyErr_NormalizeException(&Exc, &Val, &Tb);

      Str = PyObject_Str(Val);

      if ( Exc && Str )
        Trace(TRP"%s <%s: %s>", ErrorString, PyExceptionClass_Name(Exc), PyString_AS_STRING(Str));

      Py_XDECREF(Str);
      PyErr_Restore(Exc, Val, Tb);
    }
    else
      Trace(TRP"%s", ErrorString);

#endif

    if (!(Ctx.Flags & CTX_HDRS_SENT)) {
        SetStatus(Ctx, 500);
        SetClose(Ctx);
        ClearHeaders(Ctx);
        AddHeaders(Ctx, ErrorHeaders);
        SendHeaders(Ctx);

        if (Ctx.Interp && Ctx.HaveGIL) {
            if (!(Err = PyErr_Occurred()))
                WriteClient(Ctx, "%s%s%s%s%s",
                            ErrorHeader, Part1, ErrorString, Part2, Part3);
            else {
                Py_INCREF(Err); // keep reference
                WriteClient(Ctx, "%s%s%s%s", ErrorHeader, Part1, ErrorString, Part2);
                // TODO: Use something else to display the error.
                PyErr_Print();
                WriteClient(Ctx, Part3);
            }
        } else
            WriteClient(Ctx, "%s%s%s%s%s", ErrorHeader, Part1,
                        ErrorString, Part2, Part3);
    } else // Ctx.Interp always valid here...
    {
        if (!(Err = PyErr_Occurred()))
            WriteClient(Ctx, "%s%s%s%s", Part1, ErrorString, Part2, Part3);
        else {
            Py_INCREF(Err); // keep reference
            WriteClient(Ctx, "%s%s%s", Part1, ErrorString, Part2);
            PyErr_Print();
            // See above
            WriteClient(Ctx, Part3);
        }
    }

    // Add additional information
    //

    if (Err) {
        // ImportError - print sys.path
        if (PyErr_GivenExceptionMatches(Err, PyExc_ImportError)) {
            static const char Auto[] = "&lt;Auto&gt;";
            char Cwd[MAX_PATH];
            PyObject *const SysPath = PySys_GetObject("path");
            PyObject *const SysPrefix = PySys_GetObject("prefix");
            PyObject *const SysExecPrefix = PySys_GetObject("exec_prefix");
            PyObject *const SysExecutable = PySys_GetObject("executable");
            char *PythonHome = Py_GetPythonHome();
            PyObject *Sep = PyString_FromString("\r\n");
            PyObject *JoinFunc = PyObject_GetAttrString(Sep, "join");
            PyObject *Result = PyObject_CallFunctionObjArgs(JoinFunc, SysPath, NULL);

            if (Result)
                WriteClient(Ctx, "%s\r\n-- Module Search Path --\r\n\r\n%s\r\n\r\n\r\n",
                            APart1, PyString_AS_STRING(Result));
            else
                WriteClient(Ctx, "%s\r\n-- Unable to determine module search path! --\r\n\r\n\r\n",
                            APart1);

            Py_XDECREF(Result);
            Py_XDECREF(JoinFunc);
            Py_XDECREF(Sep);

            if (!PythonHome || !strlen(PythonHome)) PythonHome = (char *) Auto;
            getcwd(Cwd, MAX_PATH);

            WriteClient(Ctx, "-- Other Path Info --\r\n\r\n");
            WriteClient(Ctx, "Current Directory = '%s'\r\n", Cwd);
            WriteClient(Ctx, "Python Home       = '%s'\r\n", PythonHome);
            WriteClient(Ctx, "sys.executable    = '%s'\r\n", PyString_AsString(SysExecutable));
            WriteClient(Ctx, "sys.exec_prefix   = '%s'\r\n", PyString_AsString(SysExecPrefix));
            WriteClient(Ctx, "sys.prefix        = '%s'\r\n\r\n\r\n", PyString_AsString(SysPrefix));

            WriteClient(Ctx, strlen(APart2), APart2);

        }

        Py_DECREF(Err);
    }

    // Finish writing
    WriteFinish(Ctx);

    return HSE_STATUS_ERROR;
}
