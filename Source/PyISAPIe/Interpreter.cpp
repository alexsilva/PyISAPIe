// https://pyisapie.svn.sourceforge.net/svnroot/pyisapie/Tags/1.1.0-rc1/PyISAPIe/Interpreter.cpp
// 151 2009-05-20 20:02:46 -0700 (Wed, 20 May 2009)
// (C)2008 Phillip Sitbon <phillip@sitbon.net>
//
// Interpreter & state management functions.
//

#include "Interpreter.h"
#include "Module/Module.h"
#include "Module/Env.h"
#include "ReadWrite.h"
#include "Header.h"

// Used to determine if there is a thread state associated
// with a thread.
extern DWORD TlsIndex_Ts;
const static LPVOID THREAD_HAS_STATE = (LPVOID) 0xFFFFFFFF;

// Declared in ISAPI.cpp
// Using it here to block requests in the shutdown function,
// because allowing interpreters to finalize can release the GIL.
extern CRITICAL_SECTION CsReq;

// Interpreter objects
static PyObject *Interpreters = 0;

// The default interpreter
static Interpreter *MainInterpreter = 0;

// The interpreter map variable (valid in the main interpreter)
static PyObject *InterpreterMap = 0;

// Local function(s)
//
static PyISAPIe_Func(Interpreter *) GetInterpreter(PyObject *const);

static PyISAPIe_Func(PyThreadState *) FindThreadState(Interpreter *const);

static PyISAPIe_Func(bool) SwapThreadState(Interpreter *const);

static PyISAPIe_Func(void) _DeleteInterpreter(Interpreter *const);


/////////////////////////////////////////////////////////////////////
//  InitializeInterpreters()
//
PyISAPIe_Func(bool) InitializeInterpreters() {
    // Create the main interpreter

    MainInterpreter = new Interpreter;

    if (!MainInterpreter)
        return false;

    memset(MainInterpreter, 0, sizeof(Interpreter));
    MainInterpreter->Name = PyString_FromString("__main__");

    // Load default interpreter if it's there
    MainInterpreter->InterpreterState = PyInterpreterState_Head();

    Interpreters = PyDict_New();

    if (!Interpreters)
        return false;

    return true;
}

/////////////////////////////////////////////////////////////////////
//  AcquireInterpreter()
//
PyISAPIe_Func(Interpreter *) AcquireInterpreter(register Context &Ctx) {
    register Interpreter *Interp;

    PyEval_AcquireLock();
    Ctx.HaveGIL = true;

    if (!InterpreterMap)
        Interp = MainInterpreter;
    else {
        // The following adapted from GetEnv in Module/Env.cpp
        //
        register DWORD Size(0);

        Ctx.ECB->GetServerVariable(Ctx.ECB->ConnID,
                                   PyString_AS_STRING(InterpreterMap), NULL, &Size);

        if (Size <= 1)
            Interp = MainInterpreter;
        else {
            register PyObject *Result =
                    PyString_FromStringAndSize(NULL, Size - 1);

            Ctx.ECB->GetServerVariable(Ctx.ECB->ConnID,
                                       PyString_AS_STRING(InterpreterMap), PyString_AS_STRING(Result),
                                       &Size);

            Interp = GetInterpreter(Result);

            if (!Interp) {
                Py_DECREF(Result);
                return NULL;
            }
        }
    }

    if (!SwapThreadState(Interp))
        return NULL;

    // acquired at this point
    //

    if (!Interp->Initialized) {
        if (Interp->Initialized = InitializeInterpreter(Interp))
            if ((Interp == MainInterpreter) && InterpreterMap) {
                // Reload with properly mapped interpreter
                ReleaseInterpreter();
                return AcquireInterpreter(Ctx);
            }
    }

    return Interp;
}

/////////////////////////////////////////////////////////////////////
//  GetInterpreter()
//
PyISAPIe_Func(Interpreter *) GetInterpreter(PyObject *const Name) {
    register PyObject *Entry(PyDict_GetItem(Interpreters, Name));

    if (!Entry) {
        Interpreter *Interp(new Interpreter);

        if (!Interp)
            return NULL;

        memset(Interp, 0, sizeof(Interpreter));

        Interp->Name = Name;

        PyObject *InterpObject(PyCObject_FromVoidPtr((void *) Interp, NULL));
        PyDict_SetItem(Interpreters, Interp->Name, InterpObject);
        Py_DECREF(InterpObject);
        return Interp;
    }

    // maybe just return ((PyCObject *)self)->cobject for performance?
    return (Interpreter *) PyCObject_AsVoidPtr(Entry);
}

/////////////////////////////////////////////////////////////////////
//  FindThreadState()
//
PyISAPIe_Func(PyThreadState *) FindThreadState(Interpreter *const Interp) {
    // Locate the thread state for this thread, specific to the interpreter.
    // If it doesn't exist, create it.

    // Note: In the case of the initializing thread (DLL_PROCESS_ATTACH), the
    // thread state will be created automatically. Same for the first thread
    // state in every interpreter thereafter.

    register PyThreadState *Ts = Interp->InterpreterState->tstate_head;
    const register DWORD Id = GetCurrentThreadId();

    while (Ts) {
        if (Ts->thread_id == Id)
            return Ts;

        Ts = Ts->next;
    }

    return PyThreadState_New(Interp->InterpreterState);
}

/////////////////////////////////////////////////////////////////////
//  SwapThreadState()
//
PyISAPIe_Func(bool) SwapThreadState(Interpreter *const Interp) {
    register PyThreadState *Ts;

    if (!Interp->InterpreterState) {
        // If only using one interpreter, this shouldn't happen since
        // we use the interpreter that was created in Py_Initialize()
        // and use it as the default.

        Trace(TRP"Creating interpreter state for interpreter '%s'",
              PyString_AS_STRING(Interp->Name));
        Ts = Py_NewInterpreter();

        if (!Ts)
            return false;

        Interp->InterpreterState = Ts->interp;

        // Thread state is created with the new
        // interpreter.

        // Note: For the first thread here, we will have already created
        // a thread state for the MAIN interpreter on this thread in order
        // to determine the InterpreterMap. That's okay- it will be deleted
        // when the thread detaches.
        //

        return true;
    }

    Ts = FindThreadState(Interp);

    if (!Ts)
        return false;

    // Switch to the new/existing thread state.
    PyThreadState_Swap(Ts);
    return true;
}

/////////////////////////////////////////////////////////////////////
//  DetachThreadState()
//
PyISAPIe_Func(void) DetachThreadState() {
    // Get the Python thread states associated with this thread
    // and delete it.
    // Done for every interpreter.

    // Note that even though the delete operation is thread safe,
    // our reading of it won't be unless we can get the GIL.
    //
    // I'm having a hard time seeing this ever called. It looks like
    // IIS worker process thread pools never allow the threads to exit.
    // Maybe there is a way to have them "refresh" periodically...
    // otherwise, we'll be _Clear()ing the state from a different
    // thread on shutdown, which isn't desirable.

    if (TlsGetValue(TlsIndex_Ts) != THREAD_HAS_STATE)
        // never handled a request, so nothing to do.
        return;

    PyEval_AcquireLock();

    const register DWORD Id = GetCurrentThreadId();
    register PyInterpreterState *Is = PyInterpreterState_Head();
    register PyThreadState *Ts;

    while (Is) {
        Ts = Is->tstate_head;

        while (Ts) {
            if (Ts->thread_id == Id) {
                // If everything is done properly, there will only be this
                // thread state for this specific interpreter.
                Trace(TRP"Detaching from interpreter 0x%08X", Is);
                PyThreadState_Swap(Ts);
                PyThreadState_Clear(Ts);
                PyThreadState_Swap(NULL);
                PyThreadState_Delete(Ts);
                break;
            }
        }
    }

    PyEval_ReleaseLock();

    // Indicate this thread was "never" associated with a thread state
    TlsSetValue(TlsIndex_Ts, NULL);
    return;
}

/////////////////////////////////////////////////////////////////////
//  ReleaseInterpreter()
//
PyISAPIe_Func(void) ReleaseInterpreter() {
    PyThreadState *const Ts = PyThreadState_Swap(NULL);

    if (!Ts || (Ts->thread_id != GetCurrentThreadId())) {
        // This is condition bad.
        Py_FatalError(TRP"Thread state does not match current thread");
        return;
    }

    // UPDATE: The code here was ending the life of this thread until
    // its next use; however, Ts->dict carried information that will
    // be looked at during the next use of this object, and so any keys
    // in the dict will make objects persist until the next request.
    //
    // This is good in the sense that we can re-use connection objects
    // in a thread local manner, and bad because if this thread state is
    // never used again, the objects persist until the worker process
    // terminates. Also bad because a different thread ID picks up the
    // same thread state- this isn't bad, but not consistent behavior
    // with the rest of Python.
    //
    // See my thoughts here:
    // http://groups.google.com/group/pyisapie/browse_thread/thread/954fda71fb03f4c7?hl=en
    //

    // Now, the new behavior keeps the entire thread state intact
    // for an entirely new request. Is that really a good idea?

    // Indicate this thread is now associated with a thread state
    TlsSetValue(TlsIndex_Ts, THREAD_HAS_STATE);

    PyContext_Load(Ctx);

    if (PyContext_IsValid(Ctx))
        Ctx->HaveGIL = false;

    PyEval_ReleaseLock();
}

/////////////////////////////////////////////////////////////////////
//  InitializeInterpreter()
//
PyISAPIe_Func(bool) InitializeInterpreter(Interpreter *const Interp) {
    // TODO: Make re-callable without leakage.
    Trace(TRP"Initializing interpreter %s", PyString_AS_STRING(Interp->Name));

    ModuleInitInterp(Interp);

    //////////////////////////////////////////////
    // Load handler first, to allow for config
    // changes.
    //

    if (!Interp->RequestFunction)
        if (!LoadHandler(Interp))
            return false;


    //////////////////////////////////////////////
    // Load config module
    //

    Trace(TRP "Importing '" _PKG(CFG_MOD) "'");

    PyObject *const Config = PyImport_ImportModule(_PKG(CFG_MOD));

    if (!Config)
        return false;

    // Borrow reference
    Py_DECREF(Config);


    // Removes the module completely
#define UNLOAD_CONFIG \
    PyDict_DelItemString(Interp->InterpreterState->modules,_PKG(CFG_MOD));\
    Py_DECREF(Config);

    //////////////////////////////////////////////
    // Load config info
    //

    // InterpreterMap
    //
    if (Interp == MainInterpreter) {
        PyObject *const InterpMap(PyObject_GetAttrString(Config, "InterpreterMap"));

        if (!InterpMap || (InterpMap == Py_None)) {
            InterpreterMap = NULL;
            PyErr_Clear();
            Py_XDECREF(InterpMap);
        } else {
            if (!PyString_Check(InterpMap)) {
                Py_DECREF(InterpMap);
                UNLOAD_CONFIG;
                PyErr_SetString(PyExc_ValueError,
                                "Config parameter 'InterpreterMap' is invalid");
                return false;
            }

            InterpreterMap = InterpMap;
        }
    }

    // BufferSize
    //
    PyObject *const BufferSize(PyObject_GetAttrString(Config, "BufferSize"));

    if (!BufferSize || (BufferSize == Py_None)) {
        Interp->BufferSize = 0;
        PyErr_Clear();
        Py_XDECREF(BufferSize);
    } else {
        if (!PyInt_Check(BufferSize) ||
            PyInt_AsUnsignedLongMask(BufferSize) > MAX_BUFFER_SIZE ||
            PyInt_AsUnsignedLongMask(BufferSize) < MIN_BUFFER_SIZE) {
            Py_DECREF(BufferSize);
            UNLOAD_CONFIG;
            PyErr_SetString(PyExc_ValueError,
                            "Config parameter 'BufferSize' is invalid");
            return false;
        }

        Interp->BufferSize = PyInt_AsUnsignedLongMask(BufferSize);
        Py_DECREF(BufferSize);
    }


    // KeepAlive
    //
    PyObject *const KeepAlive(PyObject_GetAttrString(Config, "KeepAlive"));

    if (!KeepAlive || (KeepAlive == Py_True) || (KeepAlive == Py_False)) {
        Interp->KeepAlive = KeepAlive != Py_False;
        PyErr_Clear();
        Py_XDECREF(KeepAlive);
    } else {
        Py_DECREF(KeepAlive);
        UNLOAD_CONFIG;
        PyErr_SetString(PyExc_ValueError,
                        "Config parameter 'KeepAlive' is invalid");
        return false;
    }

    Py_XDECREF(KeepAlive);

    // DefaultHeaders
    //
    PyObject *const DefaultHeaders(PyObject_GetAttrString(Config, "DefaultHeaders"));

    if (!DefaultHeaders)
        PyErr_Clear();
    else {
        Py_DECREF(DefaultHeaders);
        if (!SetDefaultHeaders(Interp, DefaultHeaders)) {
            UNLOAD_CONFIG;
            return false;
        }
    }

    return true;
}

/////////////////////////////////////////////////////////////////////
//  LoadHandler()
//
PyISAPIe_Func(bool) LoadHandler(Interpreter *const Interp) {
#define UNLOAD_HANDLER \
  (Interp->RequestFunction = NULL), \
    PyDict_DelItemString(Interp->InterpreterState->modules, \
      _PKG(DISP_MOD))

    if (Interp->RequestFunction)
        UNLOAD_HANDLER;

    Trace(TRP "Importing '" _PKG(DISP_MOD) "'");

    PyObject *const Handler(PyImport_ImportModule(_PKG(DISP_MOD)));

    if (!Handler)
        return false;

    // Borrow reference
    Py_DECREF(Handler);

    Trace(TRP "Reading '" _DOT(_PKG(DISP_MOD), DISP_FN) "'");

    PyObject *const FnObject = PyObject_GetAttrString(Handler, DISP_FN);

    if (!FnObject)
        return UNLOAD_HANDLER,
                false;

    if (!FnObject->ob_type->tp_call) {
        Py_DECREF(FnObject);
        PyErr_SetString(PyExc_ValueError, "Attribute '" _DOT(_PKG(DISP_MOD), DISP_FN) "' not callable");

        return UNLOAD_HANDLER,
                false;
    }

    Interp->RequestFunction = FnObject;
    Interp->RequestFunctionPtr = FnObject->ob_type->tp_call;

    // NOT stealing reference to FnObject!

    return true;
}

/////////////////////////////////////////////////////////////////////
//  ShutdownInterpreters()
//
PyISAPIe_Func(void) ShutdownInterpreters() {

    Trace(TRP"Acquiring locks");
    EnterCriticalSection(&CsReq);
    PyEval_AcquireLock();
    Trace(TRP"Locks acquired");

    if (Interpreters) {
        // Py_Finalize does delete sub-interpreters, but this way
        // we can (re)use thread states specific to each one and
        // lay the foundation for any future cleanup necessary.
        //

        PyObject *Key, *Value;
        Py_ssize_t Pos = 0;

        while (PyDict_Next(Interpreters, &Pos, &Key, &Value))
            _DeleteInterpreter((Interpreter *) PyCObject_AsVoidPtr(Value));

        Py_CLEAR(Interpreters);
    }

    if (InterpreterMap)
        Py_CLEAR(InterpreterMap);

    if (MainInterpreter) {
        SwapThreadState(MainInterpreter);
        Py_DECREF(MainInterpreter->Name);
        Py_DECREF(MainInterpreter->RequestFunction);
        delete MainInterpreter;
        MainInterpreter = 0;
    }

    Py_Finalize();
    Trace(TRP"Py_Finalize() done");

    // Maybe continue to hold the locks, making other threads hang?
    PyEval_ReleaseLock();
    LeaveCriticalSection(&CsReq);
    return;
}

/////////////////////////////////////////////////////////////////////
//  _DeleteInterpreter()
//
//  Deletes the interpreter - only meant to be called from within
//  ShutdownInterpreters.
//
PyISAPIe_Func(void) _DeleteInterpreter(Interpreter *const Interp) {
    PyThreadState *Ts, *Ts_Tmp;

    if (!Interp->InterpreterState) {
        if (Interp->Name)
            Trace(TRP"Interpreter '%s': No state", PyString_AS_STRING(Interp->Name));
        return;
    }

    if (SwapThreadState(Interp)) {
        Ts = PyThreadState_Get();

        while (1) {
            // This isn't a great idea, but we can't kill the interpreter
            // until all the thread states except Ts are deleted.
            //
            // Note that we will be using thread states created by other
            // threads. I am guessing any Python threads running will crash.
            //
            // Also, the debug assertion will fail in PyThreadState_Swap,
            // so the conditional hack lets us debug around it.
            //

            if (Interp->InterpreterState->tstate_head != Ts)
                Ts_Tmp = Interp->InterpreterState->tstate_head;
            else if (Ts->next)
                Ts_Tmp = Ts->next;
            else
                break;

            Trace(TRP"Interpreter '%s': Clearing %i", PyString_AS_STRING(Interp->Name), Ts_Tmp->thread_id);

#ifdef Py_DEBUG
            PyInterpreterState *const Is_Tmp(Ts->interp);
            Ts->interp = NULL;
#endif

            PyThreadState_Swap(Ts_Tmp);

#ifdef Py_DEBUG
            Ts->interp = Is_Tmp;
#endif

            PyThreadState_Clear(Ts_Tmp);
            PyThreadState_Swap(NULL);
            PyThreadState_Delete(Ts_Tmp);
            PyThreadState_Swap(Ts);
        }

        // Use the standard atexit mechanism to do cleanup.
        // This is done automatically in the main interpreter.
        //
        PyObject *const Mod = PyImport_ImportModule("atexit");

        if (Mod) {
            PyObject *const Func = PyObject_GetAttrString(Mod, "_run_exitfuncs");

            if (Func) {
                Trace(TRP"Interpreter '%s': atexit._run_exitfuncs()", PyString_AS_STRING(Interp->Name));
                PyObject *const Res = PyObject_CallFunction(Func, NULL);
                Py_XDECREF(Res);
            } else
                Trace(TRP"Interpreter '%s': Function 'atexit._run_exitfuncs' not found",
                      PyString_AS_STRING(Interp->Name));

            // If the app really wants to know what happened,
            // it can hook stderr.
            PyErr_Clear();
        } else {
            Trace(TRP"Interpreter '%s': Module 'atexit' not found", PyString_AS_STRING(Interp->Name));
            PyErr_Clear();
        }

        ////

        Py_EndInterpreter(Ts);
        Trace(TRP"Interpreter '%s': Deleted", PyString_AS_STRING(Interp->Name));
        // Thread state back to NULL, GIL still held.
    }

    Py_DECREF(Interp->Name);
    Py_DECREF(Interp->RequestFunction);
    memset(Interp, 0x00, sizeof(Interpreter));
    delete Interp;
}
