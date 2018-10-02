// https://pyisapie.svn.sourceforge.net/svnroot/pyisapie/Tags/1.1.0-rc1/PyISAPIe/Header.cpp
// 151 2009-05-20 20:02:46 -0700 (Wed, 20 May 2009)
// (C)2008 Phillip Sitbon <phillip@sitbon.net>
//
// Header management.
//

#include "Header.h"

// For alloca()
#include <malloc.h>

// Status Code Map
//
inline PyISAPIe_Func(const char *const) MapStatusCode(const int &);

// Header construction
//
inline PyISAPIe_Func(bool) _SendHeaders(Context &Ctx);

/////////////////////////////////////////////////////////////////////
// AddHeaders()
//
PyISAPIe_Func(bool) AddHeaders(Context &Ctx, PyObject *Hdr) {
    if (PyString_Check(Hdr)) {
        Trace(TRP "Adding header string '%s'", PyString_AS_STRING(Hdr));
        if (!PyString_GET_SIZE(Hdr))
            return true;

        if (!Ctx.HeaderList) {
            Trace(TRP "Creating new header list");
            if (!(Ctx.HeaderList = PyList_New(1)))
                return false;

            PyList_SET_ITEM(Ctx.HeaderList, 0, Hdr);
            Py_INCREF(Hdr);
            return true;
        }

        return PyList_Append(Ctx.HeaderList, Hdr) == 0;
    } else if (!PyList_Check(Hdr) && !PyTuple_Check(Hdr))
        return PyErr_SetString(PyExc_ValueError,
                               "Header parameter must be a list, tuple, or string"),
                false;

    // TODO: Check list items to ensure they are strings

    Trace(TRP"Adding header sequence of length %i", PySequence_Length(Hdr));

    if (!Ctx.HeaderList) {
        Trace(TRP"Creating new header list");
        Ctx.HeaderList = PyList_New(0);

        if (!Ctx.HeaderList)
            return false;
    }


    return _PyList_Extend((PyListObject *) Ctx.HeaderList, Hdr) != NULL;
}

/////////////////////////////////////////////////////////////////////
// SetDefaultHeaders()
//
PyISAPIe_Func(bool) SetDefaultHeaders(Interpreter *const Interp, PyObject *Hdr) {
    bool DecRef = false;
    size_t Len;

    if (Interp->DefaultHeaders) {
        free(Interp->DefaultHeaders);
        Interp->DefaultHeaders = NULL;
    }

    if (PyString_Check(Hdr)) {
        Len = PyString_GET_SIZE(Hdr);

        Interp->DefaultHeaders = (char *) malloc(Len + 5);

        if (!Interp->DefaultHeaders)
            return PyErr_SetFromErrno(PyExc_MemoryError), false;

        memcpy(Interp->DefaultHeaders, PyString_AS_STRING(Hdr), Len);
        memcpy(&Interp->DefaultHeaders[Len], "\r\n\r\n\0", 5);
        Interp->DefaultHeaderLength = (unsigned long) Len + 4;
        return true;
    }

    if (PyList_Check(Hdr)) {
        Hdr = PyList_AsTuple(Hdr);
        DecRef = true;

        if (!Hdr)
            return false;
    } else if (!PyTuple_Check(Hdr)) {
        PyErr_SetString(PyExc_ValueError,
                        "Parameter must be a list, tuple, or string");
        return false;
    }

    PyObject *Item = NULL;
    const int Count = PyTuple_GET_SIZE(Hdr);
    int N = 0;
    Len = 0;

    for (N; N < Count; ++N) {
        Item = PyTuple_GET_ITEM(Hdr, N);

        if (!PyString_Check(Item)) {
            PyErr_SetString(PyExc_ValueError,
                            "Sequence items must be strings");
            if (DecRef) Py_DECREF(Hdr);
            return false;
        }

        if (!PyString_GET_SIZE(Item))
            continue;

        Len += PyString_GET_SIZE(Item) + 2;
    }


    if (!Len) {
        Len = 2;
        Interp->DefaultHeaders = (char *) malloc(5);
        strcpy(Interp->DefaultHeaders, "\r\n");
    } else {
        Interp->DefaultHeaders = (char *) malloc(Len + 5);
        Interp->DefaultHeaders[0] = 0;
    }

    Interp->DefaultHeaderLength = (unsigned long) Len + 2;


    if (!Interp->DefaultHeaders) {
        if (DecRef) Py_DECREF(Hdr);
        return PyErr_SetFromErrno(PyExc_MemoryError), false;
    }

    for (N = 0; N < Count; ++N) {
        if (!PyString_GET_SIZE(Item))
            continue;

        strcat(Interp->DefaultHeaders, PyString_AS_STRING(PyTuple_GET_ITEM(Hdr, N)));
        strcat(Interp->DefaultHeaders, "\r\n");
    }

    strcat(Interp->DefaultHeaders, "\r\n");

    if (DecRef)
        Py_DECREF(Hdr);

    return true;
}

/////////////////////////////////////////////////////////////////////
// SetStatus()
//
PyISAPIe_Func(bool) SetStatus(Context &Ctx, const int &Status) {
    if (const char *const StatusStr = MapStatusCode(Status)) {
        Ctx.ECB->dwHttpStatusCode = (DWORD) Status;
        Ctx.Headers.pszStatus = StatusStr;
        Ctx.Headers.cchStatus = (unsigned long) strlen(StatusStr);
        return true;
    }

    return PyErr_SetString(PyExc_ValueError, "Invalid status code"),
            false;
}

/////////////////////////////////////////////////////////////////////
// SetLength()
//
PyISAPIe_Func(bool) SetLength(Context &Ctx, const int &Length) {
    if (Ctx.ContentLength != -1)
        return PyErr_SetString(PyExc_ValueError, "Content length already set"),
                false;

    if (Length >= -1) {
        Ctx.ContentLength = Length;
        Ctx.Flags &= ~CTX_CHUNKED;
        Ctx.RealBuffer = Ctx.Buffer = NULL;
        Ctx.RealBufferSize = 0;
        return true;
    }

    return PyErr_SetString(PyExc_ValueError, "Invalid content length"),
            false;
}

/////////////////////////////////////////////////////////////////////
// SetClose()
//
PyISAPIe_Func(bool) SetClose(Context &Ctx) {
    if (Ctx.Flags & CTX_KEEPALIVE) {
        Ctx.Flags &= ~CTX_KEEPALIVE;
        Ctx.Headers.fKeepConn = FALSE;

        if (Ctx.Flags & CTX_CHUNKED) {
            Ctx.Flags &= ~CTX_CHUNKED;
            // No longer doing chunked encoding:
            Ctx.RealBuffer = Ctx.Buffer;
            Ctx.RealBufferSize = Ctx.Interp->BufferSize;
        }
    }

    return true;
}

/////////////////////////////////////////////////////////////////////
// ClearHeaders()
//
PyISAPIe_Func(void) ClearHeaders(Context &Ctx) {
    if (Ctx.HeaderList) {
        const int Count = PyList_GET_SIZE(Ctx.HeaderList);

        for (int N(0); N < Count; ++N) {
            if (PyList_GET_ITEM(Ctx.HeaderList, N)) {
                Py_DECREF(PyList_GET_ITEM(Ctx.HeaderList, N));
                PyList_SET_ITEM(Ctx.HeaderList, N, NULL);
            }
        }

        Py_DECREF(Ctx.HeaderList);
        Ctx.HeaderList = NULL;
    }
}

/////////////////////////////////////////////////////////////////////
// ConstructHeaders()
//
PyISAPIe_Func(bool) SendHeaders(Context &Ctx) {
    static const char ChunkHdr[] = "Transfer-Encoding: chunked\r\n";
    static const int ChunkHdrLen = 28;
    char *All;
    char *ExHdr = NULL;
    int xLen = 0;

    if (Ctx.ContentLength >= 0) {
        xLen = _scprintf("Content-Length: %i\r\n", Ctx.ContentLength);
        ExHdr = (char *) alloca(xLen + 1);
        sprintf(ExHdr, "Content-Length: %i\r\n", Ctx.ContentLength);
    } else if (Ctx.Flags & CTX_KEEPALIVE) {
        if (Ctx.Flags & CTX_CHUNKED) {
            xLen = ChunkHdrLen;
            ExHdr = (char *) ChunkHdr;
        } else {
            Ctx.Flags &= ~CTX_KEEPALIVE;
            Ctx.Headers.fKeepConn = FALSE;
        }
    }

    if (!Ctx.HeaderList) {
        Trace(TRP"Sending default headers");

        if (xLen) {
            Ctx.Headers.cchHeader = xLen + Ctx.Interp->DefaultHeaderLength;
            All = (char *) alloca(Ctx.Headers.cchHeader + 2);
            memcpy(All, ExHdr, xLen);
            memcpy(&All[xLen], Ctx.Interp->DefaultHeaders,
                   Ctx.Interp->DefaultHeaderLength + 1);
            Ctx.Headers.pszHeader = All;
        }

        // I'm wondering if a tail-call here would be optimized such that
        // the allocated stack data is no longer valid?
        //
        return _SendHeaders(Ctx);
    }

    Trace(TRP"Sending headers");

    const int Count = PyList_GET_SIZE(Ctx.HeaderList);
    PyObject *Item;
    int Len = 0;
    int N;

    // TODO: Look at PySequence_Fast & PySequence_Fast_GET_ITEM

    for (N = 0; N < Count; ++N) {
        Item = PyList_GET_ITEM(Ctx.HeaderList, N);

        if (!PyString_GET_SIZE(Item))
            continue;

        Len += PyString_GET_SIZE(Item) + 2;
    }

    if (xLen) {
        Len += xLen;
        Ctx.Headers.cchHeader = Len + 2;
        All = (char *) alloca(Len + 3);
        memcpy(All, ExHdr, xLen);
        Len = xLen;
    } else {
        Ctx.Headers.cchHeader = Len + 2;
        All = (char *) alloca(Len + 3);
        Len = 0;
    }

    Ctx.Headers.pszHeader = All;


    // Acquire the lock: Py_DECREF can delete objects.
    PyContext_Async_Begin(&Ctx);

        for (N = 0; N < Count; ++N) {
            Item = PyList_GET_ITEM(Ctx.HeaderList, N);

            if (PyString_GET_SIZE(Item)) {
                memcpy(&All[Len], PyString_AS_STRING(Item), PyString_GET_SIZE(Item));
                Len += PyString_GET_SIZE(Item);
                memcpy(&All[Len], "\r\n", 2);
                Len += 2;
            }

            // Not setting the items to NULL causes problems
            Py_DECREF(Item);
            PyList_SET_ITEM(Ctx.HeaderList, N, NULL);
        }

        memcpy(&All[Len], "\r\n\0", 3);

        Py_DECREF(Ctx.HeaderList);
        Ctx.HeaderList = NULL;

    PyContext_Async_End(&Ctx);

    return _SendHeaders(Ctx);
}

/////////////////////////////////////////////////////////////////////
// _SendHeaders()
//
inline PyISAPIe_Func(bool) _SendHeaders(Context &Ctx) {
    if (Ctx.ECB->ServerSupportFunction(Ctx.ECB->ConnID,
                                       HSE_REQ_SEND_RESPONSE_HEADER_EX,
                                       (LPVOID) &Ctx.Headers, NULL, NULL)) {
        Ctx.Flags |= CTX_HDRS_SENT;
        return true;
    } else // TODO: Use HDRS_FAILED flag, extended info in Fail()?
        Trace(TRP"Header send failed (0x%08X)", GetLastError());

    return false;
}

/////////////////////////////////////////////////////////////////////
// MapStatusCode()
//
inline PyISAPIe_Func(const char *const) MapStatusCode(const int &Status) {
    switch (Status) {
        // common numbers first
        case 200:
            return "200 OK";
        case 404:
            return "404 Not Found";
        case 403:
            return "403 Forbidden";
        case 500:
            return "500 Internal Server Error";
            //
        case 100:
            return "100 Continue";
        case 101:
            return "101 Switching Protocols";
        case 201:
            return "201 Created";
        case 202:
            return "202 Accepted";
        case 203:
            return "203 Non-Authoritative Information";
        case 204:
            return "204 No Content";
        case 205:
            return "205 Reset Content";
        case 206:
            return "206 Partial Content";
        case 300:
            return "300 Multiple Choices";
        case 301:
            return "301 Moved Permanently";
        case 302:
            return "302 Moved Temporarily";
        case 303:
            return "303 See Other";
        case 304:
            return "304 Not Modified";
        case 305:
            return "305 Use Proxy";
        case 307:
            return "307 Temporary Redirect";
        case 400:
            return "400 Bad Request";
        case 401:
            return "401 Not Authorized";
        case 402:
            return "402 Payment Required";
        case 405:
            return "405 Method Not Allowed";
        case 406:
            return "406 Not Acceptable";
        case 407:
            return "407 Proxy Authentication Required";
        case 408:
            return "408 Request Timeout";
        case 409:
            return "409 Conflict";
        case 410:
            return "410 Gone";
        case 411:
            return "411 Length Required";
        case 412:
            return "412 Precondition Failed";
        case 413:
            return "413 Request Entity Too Large";
        case 414:
            return "414 Request-URI Too Long";
        case 415:
            return "415 Unsupported Media Type";
        case 416:
            return "416 Requested Range Not Satisfiable";
        case 417:
            return "417 Expectation Failed";
        case 501:
            return "501 Not Implemented";
        case 502:
            return "502 Bad Gateway";
        case 503:
            return "503 Service Unavailable";
        case 504:
            return "504 Gateway Timeout";
        case 505:
            return "505 HTTP Version Not Supported";
        default:
            return NULL;
    }
}
