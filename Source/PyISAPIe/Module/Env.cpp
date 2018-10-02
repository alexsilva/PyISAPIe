// https://pyisapie.svn.sourceforge.net/svnroot/pyisapie/Tags/1.1.0-rc1/PyISAPIe/Module/Env.cpp
// 150 2009-05-20 19:44:28 -0700 (Wed, 20 May 2009)
// (C)2008 Phillip Sitbon <phillip@sitbon.net>
//
// A "smart" interface to GetServerVariable.
//
#include "Env.h"

// For alloca()
#include <malloc.h>

// Handler for tp_getattro
//
static PyObject *GetEnv(PyObject *, PyObject *);

// The Env object, with no extra information
//
struct EnvObject {
    PyObject_HEAD
};

// The Env type object
//
static PyTypeObject EnvType =
        {
                PyObject_HEAD_INIT(NULL)
                0,                              /*ob_size*/
                "PyISAPIe.Env",                 /*tp_name*/
                sizeof(EnvObject),              /*tp_basicsize*/
                0,                              /*tp_itemsize*/
                0,                              /*tp_dealloc*/
                0,                              /*tp_print*/
                0,                              /*tp_getattr*/
                0,                              /*tp_setattr*/
                0,                              /*tp_compare*/
                0,                              /*tp_repr*/
                0,                              /*tp_as_number*/
                0,                              /*tp_as_sequence*/
                0,                              /*tp_as_mapping*/
                0,                              /*tp_hash */
                0,                              /*tp_call*/
                0,                              /*tp_str*/
                GetEnv,                         /*tp_getattro*/
                0,                              /*tp_setattro*/
                0,                              /*tp_as_buffer*/
                Py_TPFLAGS_DEFAULT,             /*tp_flags*/
                "Server environment interface", /* tp_doc */
                0,                                  /* tp_traverse */
                0,                                  /* tp_clear */
                0,                                  /* tp_richcompare */
                0,                                  /* tp_weaklistoffset */
                0,                                  /* tp_iter */
                0,                                  /* tp_iternext */
                0,                              /* tp_methods */
                0,                              /* tp_members */
                0,                              /* tp_getset */
                0,                              /* tp_base */
                0,                              /* tp_dict */
                0,                              /* tp_descr_get */
                0,                              /* tp_descr_set */
                0,                              /* tp_dictoffset */
                0,                              /* tp_init */
                0,                              /* tp_alloc */
                PyType_GenericNew,              /* tp_new */
        };

// The object instance
//
static EnvObject *Instance = 0;

// Empty string
//
static PyObject *Empty = 0;

// To avoid deleting it
//
#define Py_RETURN_EMPTY return Py_INCREF(Empty), Empty

/////////////////////////////////////////////////////////////////////
// EnvInitSystem()
//
PyISAPIe_Func(void) EnvInitSystem() {
    PyType_Ready(&EnvType);
}

/////////////////////////////////////////////////////////////////////
// EnvInitModule()
//
PyISAPIe_Func(void) EnvInitModule(PyObject *Module) {
    if (!Instance)
        if (!(Instance = PyObject_New(EnvObject, &EnvType)))
            return;

    if (!Empty)
        if (!(Empty = PyString_FromString("")))
            return;

    PyObject_SetAttrString(Module, "Env", (PyObject *) Instance);
}

/////////////////////////////////////////////////////////////////////
// GetEnv()
//
PyObject *GetEnv(PyObject *, PyObject *Name) {
    // Warning: No type checking!
    // Update: Don't need it - getattr() type checks (I think?)

    PyContext_LoadAndCheck(Ctx, NULL)

    register DWORD Err;
    register DWORD Size(0);

    Trace(TRP"Getting Env attribute '%s'", PyString_AS_STRING(Name));

    Ctx->ECB->GetServerVariable(Ctx->ECB->ConnID,
                                PyString_AS_STRING(Name), NULL, &Size);

    // should be an error
    if ((Err = GetLastError()) != ERROR_INSUFFICIENT_BUFFER) {
        // Check for "extra" options
        if (Err == ERROR_INVALID_INDEX) {

            if (!strcmp(PyString_AS_STRING(Name), "KeepAlive")) {
                if (Ctx->Flags & CTX_KEEPALIVE)
                    Py_RETURN_TRUE;

                Py_RETURN_FALSE;
            }

            if (!strcmp(PyString_AS_STRING(Name), "DataSent")) {
                if (Ctx->Flags & CTX_DATA_SENT)
                    Py_RETURN_TRUE;

                Py_RETURN_FALSE;
            }

            if (!strcmp(PyString_AS_STRING(Name), "HeadersSent")) {
                if (Ctx->Flags & CTX_HDRS_SENT)
                    Py_RETURN_TRUE;

                Py_RETURN_FALSE;
            }

            if (!strcmp(PyString_AS_STRING(Name), "SCRIPT_TRANSLATED")) {
                // Will only get here if we're on IIS 5, because 6+ does this for us.

                Ctx->ECB->GetServerVariable(Ctx->ECB->ConnID, "SCRIPT_NAME", NULL, &Size);
                if (Size <= 1)
                    Py_RETURN_EMPTY;

                char *const Script = (char *) alloca(Size);

                Ctx->ECB->GetServerVariable(Ctx->ECB->ConnID, "SCRIPT_NAME", Script, &Size);

                register DWORD nSize(Size);

                Ctx->ECB->ServerSupportFunction(Ctx->ECB->ConnID, HSE_REQ_MAP_URL_TO_PATH,
                                                Script, &nSize, NULL);

                if (nSize <= 1)
                    Py_RETURN_EMPTY;

                PyObject *const PyScr = PyString_FromStringAndSize(NULL, nSize - 1);
                memcpy(PyString_AS_STRING(PyScr), Script, Size);

                Ctx->ECB->ServerSupportFunction(Ctx->ECB->ConnID, HSE_REQ_MAP_URL_TO_PATH,
                                                PyString_AS_STRING(PyScr), &nSize, NULL);

                return PyScr;
            }

            // Fall through
        }

        // 0x00000585 = 1413L = ERROR_INVALID_INDEX
        if (Err != ERROR_INVALID_INDEX) {
            Trace(TRP"Getting Env attribute '%s': Error(1) 0x%08X", PyString_AS_STRING(Name), Err);
            return PyErr_SetFromWindowsErr(Err);
        } else
            SetLastError(0);

        Py_RETURN_EMPTY;
        //return PyErr_SetFromWindowsErr(Err);
    } else
        SetLastError(0);

    if (Size <= 1)
        Py_RETURN_EMPTY;

    register PyObject *const Result = PyString_FromStringAndSize(NULL, Size - 1);

    if (!Result) {
        Trace(TRP"Getting Env attribute '%s': %i-byte alloc failed", PyString_AS_STRING(Name), Size);
        return NULL;
    }

    if (Ctx->ECB->GetServerVariable(Ctx->ECB->ConnID, PyString_AS_STRING(Name), PyString_AS_STRING(Result), &Size) ==
        FALSE) {
        Trace(TRP"Getting Env attribute '%s': Error(2) 0x%08X", PyString_AS_STRING(Name), GetLastError());
        return PyErr_SetFromWindowsErr(0);
    }


    return Result;
}
