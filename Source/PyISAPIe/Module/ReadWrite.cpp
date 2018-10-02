// https://pyisapie.svn.sourceforge.net/svnroot/pyisapie/Tags/1.1.0-rc1/PyISAPIe/Module/ReadWrite.cpp
// 151 2009-05-20 20:02:46 -0700 (Wed, 20 May 2009)
// (C)2008 Phillip Sitbon <phillip@sitbon.net>
//
// Implements stdin/stdout-like objects for data transfer.
// Adapted from ModuleReadWrite.cpp.
//
// These were left as modules, but the softspace attribute
// was causing problems so they got converted to objects
// so that the softspace attribute will never exist.
//
#include "ReadWrite.h"
#include "../ReadWrite.h"
#include "../PyISAPIe.h"

// For alloca()
#include <malloc.h>

// Object types
//
struct ReadClientObject {
    PyObject_HEAD
};
struct WriteClientObject {
    PyObject_HEAD
    int softspace;
};

// Object instances
//
static ReadClientObject *ReadObj = NULL;
static WriteClientObject *WriteObj = NULL;

// Frequently used strings
//
//static PyObject *Str_stdin     = PyString_FromString("stdin");
//static PyObject *Str_stdout    = PyString_FromString("stdout");
//static PyObject *Str_stderr    = PyString_FromString("stderr");

// Type method struct defs
//

static PyMethodDef ReadClientMethods[] = {

        {"read", (PyCFunction) ReadClient, METH_VARARGS,
                "Read client data (See PyISAPIE.Read)"},
        {NULL, NULL, 0, NULL}
};

static PyMethodDef WriteClientMethods[] = {

        {"write", (PyCFunction) WriteClient, METH_O,
                "Write to output (See PyISAPIe.Write)"},

        {NULL, NULL, 0, NULL}
};

static PyMemberDef WriteClientMembers[] = {
        {"softspace", T_INT, offsetof(WriteClientObject, softspace),
                0, "softspace parameter"},

        {NULL}
};

// Type defs
//

static PyTypeObject ReadClientType =
        {
                PyObject_HEAD_INIT(NULL) 0, "PyISAPIe.ReadClientObject",
                sizeof(ReadClientObject),
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, Py_TPFLAGS_DEFAULT,
                "stdin proxy", 0, 0, 0, 0, 0, 0, ReadClientMethods,
                0, 0, 0, 0, 0, 0, 0, 0, 0, PyType_GenericNew,
        };

static PyTypeObject WriteClientType =
        {
                PyObject_HEAD_INIT(NULL) 0, "PyISAPIe.WriteClientObject",
                sizeof(WriteClientType),
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, Py_TPFLAGS_DEFAULT,
                "stdout/stderr proxy", 0, 0, 0, 0, 0, 0, WriteClientMethods,
                WriteClientMembers, 0, 0, 0, 0, 0, 0, 0, 0, PyType_GenericNew,
        };

// stderr for trace
//
#ifdef TRACE_LOG

PyObject *ErrWriteClient( PyObject *, PyObject * );

struct ErrWriteClientObject  { PyObject_HEAD int softspace; };

static ErrWriteClientObject * ErrWriteObj = NULL;

static PyMethodDef ErrWriteClientMethods[] = {

    {"write", (PyCFunction) ErrWriteClient, METH_O,
     "Write to trace output (See PyISAPIe.Write)"},

    {NULL, NULL, 0, NULL}
};

static PyMemberDef ErrWriteClientMembers[] = {
    {"softspace", T_INT, offsetof(ErrWriteClientObject, softspace),
      0, "softspace parameter"},

    {NULL}
};

static PyTypeObject ErrWriteClientType =
{
  PyObject_HEAD_INIT(NULL) 0, "PyISAPIe.ErrWriteClientObject",
  sizeof(ErrWriteClientType),
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,Py_TPFLAGS_DEFAULT,
  "stderr trace proxy",0,0,0,0,0,0,ErrWriteClientMethods,
  ErrWriteClientMembers,0,0,0,0,0,0,0,0,PyType_GenericNew,
};

#endif

/////////////////////////////////////////////////////////////////////
// ReadWriteInitInterp()
//
PyISAPIe_Func(void) ReadWriteInitInterp(Interpreter *const Interp) {
    if (!ReadObj) {
        Trace(TRP"Creating ReadClientObject instance");
        PyType_Ready(&ReadClientType);
        ReadObj = PyObject_New(ReadClientObject, &ReadClientType);
    }

    if (!WriteObj) {
        Trace(TRP"Creating WriteClientObject instance");
        PyType_Ready(&WriteClientType);
        WriteObj = PyObject_New(WriteClientObject, &WriteClientType);

    }

#ifdef TRACE_LOG

    if ( !ErrWriteObj )
    {
      Trace(TRP"Creating ErrWriteClientObject instance");
      PyType_Ready(&ErrWriteClientType);
      ErrWriteObj = PyObject_New(ErrWriteClientObject, &ErrWriteClientType);

    }

#endif

    // TODO: replace __stderr__ when tracing to prevent loss of cleanup traces.
    Trace(TRP"Replacing stdin/stdout/stderr");

    // Multiple call side-effect: The modules' refcounts increase

    //PyDict_SetItem(Ctx.Interp->InterpreterState->sysdict,
    //  Str_stdin,  (PyObject *)ReadObj);
    //PyDict_SetItem(Ctx.Interp->InterpreterState->sysdict,
    //  Str_stdout, (PyObject *)WriteObj);
    //PyDict_SetItem(Ctx.Interp->InterpreterState->sysdict,
    //  Str_stderr, (PyObject *)WriteObj);

    PyDict_SetItemString(Interp->InterpreterState->sysdict,
                         "stdin", (PyObject *) ReadObj);
    PyDict_SetItemString(Interp->InterpreterState->sysdict,
                         "stdout", (PyObject *) WriteObj);

#ifndef TRACE_LOG
    PyDict_SetItemString(Interp->InterpreterState->sysdict,
                         "stderr", (PyObject *) WriteObj);
#else
    PyDict_SetItemString(Interp->InterpreterState->sysdict,
      "stderr", (PyObject *)ErrWriteObj);
#endif
}

/////////////////////////////////////////////////////////////////////
// ReadWriteBeginRequest()
//
PyISAPIe_Func(void) ReadWriteBeginRequest(Context &Ctx) {
    // To ensure they haven't been changed - it seems that in some
    // of the endurance tests (> 50000 requests) that output is
    // magically reset to stdout/stderr.
    // It would be nice to know why/how.
    //

    // UPDATE:
    //   This doesn't seem to be happening anymore. Will verify on
    //   IIS 5.1 before complete removal.

    //PyDict_SetItem(Ctx.Interp->InterpreterState->sysdict,
    //  Str_stdin, ReadModule);
    //PyDict_SetItem(Ctx.Interp->InterpreterState->sysdict,
    //  Str_stdout, WriteModule);
    //PyDict_SetItem(Ctx.Interp->InterpreterState->sysdict,
    //  Str_stderr, WriteModule);
}

/////////////////////////////////////////////////////////////////////
//  ReadClient()
//
PyObject *ReadClient(PyObject *, PyObject *Args) {
    long Bytes = 0;
    PyObject *Data;

    if (!PyArg_ParseTuple(Args, "|k", &Bytes))
        return NULL;

    PyContext_LoadAndCheck(Ctx, NULL)

    if (Bytes < 0)
        // Return total size of data available
        return PyInt_FromLong((long) ReadClient(*Ctx, 0, NULL));

    Bytes = (long) ReadClient(*Ctx, Bytes, NULL);
    Data = PyString_FromStringAndSize(NULL, (DWORD) Bytes);

    if (!Data)
        return NULL;

    // XXX: is it ok not to have an intermediate buffer?
    // it does break the rules, but can it cause any problems?
    //
    // UPDATE: I don't think there's a problem; nobody has access
    // to the newly created string, so we should be ok -
    // no API call are made while the lock is not held.
    //

    PyThreadState *Ts_Save = PyContext_Unblock(Ctx);

    if (ReadClient(*Ctx, (DWORD) Bytes, PyString_AS_STRING(Data)) != (DWORD) Bytes) {
        PyContext_Block(Ctx, Ts_Save);
        Py_DECREF(Data);
        return PyErr_SetFromWindowsErr(0);
    }

    PyContext_Block(Ctx, Ts_Save);
    return Data;
}

/////////////////////////////////////////////////////////////////////
//  WriteClient()
//
PyObject *WriteClient(PyObject *, PyObject *Arg) {
    // TODO: Possibly add format parameter support

    //char *Data = NULL;
    //unsigned long Length = 0;
    //if ( !PyArg_ParseTuple(Args, "s#", &Data, (int *)&Length) || !Data )
    //  return NULL;

    if (!PyString_Check(Arg))
        return PyErr_SetString(PyExc_ValueError, "Parameter to Write() must be a string"),
                false;

    const char *const Data(PyString_AS_STRING(Arg));
    const unsigned long Length((unsigned long) PyString_GET_SIZE(Arg));

    if ((int) Length <= 0)
        // Fail silently
        Py_RETURN_NONE;

    PyContext_LoadAndCheck(Ctx, NULL)

    if (Ctx->ContentLength >= 0) {
        if (Ctx->Flags & CTX_DATA_SENT)
            return PyErr_SetString(PyExc_ValueError,
                                   "Data specified by content length already sent"),
                    false;

        if ((Length != Ctx->ContentLength))
            return PyErr_SetString(PyExc_ValueError,
                                   "Data length does not match specified content length"),
                    false;
    }

    // XXX: see comment above.
    //
    // UPDATE: This is a little more tricky, because there is the
    // possibility that Data is being used by another thread. Since
    // strings are immutable, we can only hope that it doesn't
    // become invalid after the lock is released. Further testing
    // should reveal any bugs in this.
    //
    // New development: The SendHeaders() function might get called,
    // and after looking at listobject.c, there seems to be a place
    // where freeing the list is not at all safe because empty lists
    // are kept in a global cache. More checking will be done to
    // see if it ever chokes.
    //
    // I was finally able to catch a race condition, specifically with
    // the header list being deleted via Py_DECREF in SendHeaders().
    // There are two options: (1) Re-acquire the GIL in SendHeaders() or
    // (2) just don't release it here. Even sending 14kB data, WriteClient
    // only takes a metter of microseconds. Getting the GIL is closer
    // to milliseconds, so I need to study the implications of the
    // options. Possibly unlocking when the size is over a certain
    // threshold is in order. Also, my WriteClient measurements are
    // cheats so far, because I am sending to localhost. Will do more
    // testing over the wire.
    //

    PyThreadState *Ts_Save = PyContext_Unblock(Ctx);

    if (!WriteClient(*Ctx, (DWORD) Length, Data)) {
        PyContext_Block(Ctx, Ts_Save);
        return PyErr_SetFromWindowsErr(0);
    }

    PyContext_Block(Ctx, Ts_Save);

    Py_RETURN_NONE;
}

/////////////////////////////////////////////////////////////////////
//  ErrWriteClient()
//
#ifdef TRACE_LOG
PyObject *ErrWriteClient( PyObject *, PyObject *Arg )
{
  char *s = PyString_AS_STRING(Arg);
  int len;
  if ( !s || !(len = strlen(s)) )
    Py_RETURN_NONE;

  if ( s[len-1] == '\n' )
    s[len-1] = '\0';

  Trace("[stderr] %s", s);
  Py_RETURN_NONE;
}

#endif