// https://pyisapie.svn.sourceforge.net/svnroot/pyisapie/Tags/1.1.0-rc1/PyISAPIe/Module/Module.cpp
// 151 2009-05-20 20:02:46 -0700 (Wed, 20 May 2009)
// (C)2008 Phillip Sitbon <phillip@sitbon.net>
//
// The PyISAPIe python module implementation.
//
#include "Module.h"
#include "Env.h"
#include "ReadWrite.h"
#include "Doc.h"
#include "../Header.h"
#include "../PyISAPIe.h"

#include <malloc.h>

// The module init function -
// export to make sure 'import PyISAPIe' works.
//

// Local functions
//
static PyObject *Header(PyObject *, PyObject *, PyObject *);

// Module method def struct
static PyMethodDef PyISAPIeMethods[] = {

        {"Header", (PyCFunction) Header,      METH_VARARGS | METH_KEYWORDS, Doc::Header},
        {"Read",   (PyCFunction) ReadClient,  METH_VARARGS, Doc::Read},
        {"Write",  (PyCFunction) WriteClient, METH_O,       Doc::Write},

        {NULL, NULL, 0,                                                     NULL}
};

// Defined in Main.cpp
//
extern char ModuleFile[];
extern char ModulePath[];

/////////////////////////////////////////////////////////////////////
// ModuleInitSystem()
//
PyISAPIe_Func(void) ModuleInitSystem() {
    // GIL not acquired here, nor has Py_Initialize() been called.

    if (PyImport_AppendInittab("PyISAPIe", initPyISAPIe) == -1)
        return;

    return EnvInitSystem();
}

/////////////////////////////////////////////////////////////////////
// ModuleInitInterp()
//
PyISAPIe_Func(void) ModuleInitInterp(Interpreter *const Interp) {
    _Py_CheckInterval = DEFAULT_CHECK_INTERVAL;

    PyObject *Executable = PyString_FromString(ModuleFile);

    PySys_SetObject("executable", Executable);

    Py_DECREF(Executable);

    //Trace(TRP"Deleting __main__ module");
    //PyDict_DelItemString(Interp->InterpreterState->modules, "__main__");
    //PyErr_Clear(); // just in case it's done twice

    //docs say no exception raised if key isn't there
    if (!PyDict_GetItemString(PyEval_GetBuiltins(), INTERP_VAR)) {
        //Trace(TRP"Setting __builtins__." INTERP_VAR);
        PyDict_SetItemString(PyEval_GetBuiltins(), INTERP_VAR,
                             Interp->Name);
    }

    // Add the dll file's path to the front of the import list if
    // it isn't there. It should be already now that the global init
    // sets sys.executable. It is moved to the front either way.
    //
    // From what I understand, sys.path values never have a trailing
    // slash --- neither will ModulePath.

    int Idx;

    PyObject *PyModulePath = PyString_FromString(ModulePath);
    PyObject *const SysPath = PySys_GetObject("path");

    if (!PyModulePath || !SysPath)
        return;

    if ((Idx = PySequence_Index(SysPath, PyModulePath)) < 0) {
        PyErr_Clear();
        Trace(TRP"Adding '%s' to import path", ModulePath);
        PyList_Insert(SysPath, 0, PyModulePath);
    } else if (Idx != 0) {
        Trace(TRP"Moving '%s' to head of import path", ModulePath);
        PySequence_DelItem(SysPath, Idx);
        PyList_Insert(SysPath, 0, PyModulePath);
    }

    Py_DECREF(PyModulePath);

    return ReadWriteInitInterp(Interp);
}

/////////////////////////////////////////////////////////////////////
// ModuleBeginRequest()
//
PyISAPIe_Func(void) ModuleBeginRequest(Context &Ctx) {
    return ReadWriteBeginRequest(Ctx);
}

/////////////////////////////////////////////////////////////////////
// initPyISAPIe()
//
PyMODINIT_FUNC initPyISAPIe() {
    PyObject *mod = Py_InitModule3("PyISAPIe", PyISAPIeMethods, Doc::Module);
    if (!mod) return;
    return EnvInitModule(mod);
}

/////////////////////////////////////////////////////////////////////
// Header()
//
PyObject *Header(PyObject *, PyObject *Args, PyObject *Kwds) {
    PyObject *Arg = NULL;
    PyObject *Close = NULL;
    int Status = 0;
    int Length = -1;

    const static char *Keywords[] = {"Header", "Status", "Length", "Close", NULL};

    if (!PyArg_ParseTupleAndKeywords(Args, Kwds, "|OiiO", (char **) Keywords,
                                     &Arg, &Status, &Length, &Close))
        return NULL;

    PyContext_LoadAndCheck(Ctx, NULL)

    if (Ctx->Flags & CTX_HDRS_SENT) {
        PyErr_SetString(PyExc_Exception, "Headers already sent");
        return NULL;
    }

    if (Status)
        if (!SetStatus(*Ctx, Status))
            return NULL;

    if (Length >= 0)
        if (!SetLength(*Ctx, Length))
            return NULL;

    if (Close == Py_True) {
        if (!SetClose(*Ctx))
            return NULL;
    } else if (Close) {
        PyErr_SetString(PyExc_ValueError, "Close parameter must be True when specified");
        return NULL;
    }

    if (Arg && !AddHeaders(*Ctx, Arg))
        return NULL;

    Py_RETURN_NONE;
}
