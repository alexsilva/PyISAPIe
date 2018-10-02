// https://pyisapie.svn.sourceforge.net/svnroot/pyisapie/Tags/1.1.0-rc1/PyISAPIe/Main.cpp
// 149 2009-05-18 22:26:25 -0700 (Mon, 18 May 2009)
// (C)2008 Phillip Sitbon <phillip@sitbon.net>
//
// Handles DLL initialization & shutdown.
//
#include "PyISAPIe.h"
#include "Module/Module.h"
#include "Interpreter.h"

#ifdef PyISAPIe_WithPythonCore

extern "C" {
  // Some differences come up during optimization with the win32
  // Intel compiler.
#if defined(__INTEL_COMPILER)
#define __export __declspec(dllexport)
#else
#define __export
#endif

  // For building with the Python DLL
  //
  __export const char *PyWin_DLLVersionString = PY_VERSION;
  __export HANDLE PyWin_DLLhModule;
}

#endif // PyISAPIe_WithPythonCore

// For whatever needs it - NOTE: Python only allows length up to MAX_PATH.
char ModuleFile[2048];
char ModulePath[2048];

// For context info and thread state
DWORD TlsIndex_Ctx;
DWORD TlsIndex_Ts;

// For one-time initialization
static unsigned int Initialized = 0;

extern CRITICAL_SECTION CsReq;

/////////////////////////////////////////////////////////////////////
//  DLLMain()
//
//  - Some DllMain rules are broken here, but there doesn't seem
//    to be any problems. If anything comes up, this code can be
//    moved to GetExtensionVersion().
//
BOOL WINAPI DllMain(HINSTANCE hInst, DWORD Reason, LPVOID) {
    SAFE_BEGIN
    switch (Reason) {
        case DLL_PROCESS_ATTACH: {
            Trace(TRP"Attached 0x%08X (%i)", hInst, Initialized);

            if (Initialized++) {
                Trace(TRP"Warning: PyISAPIe already initialized!");
                break;
            }

#ifdef PyISAPIe_WithPythonCore
            PyWin_DLLhModule = (HANDLE) hInst;
#endif

            // Get a TLS index for the context and thread state
            TlsIndex_Ctx = TlsAlloc();
            TlsIndex_Ts = TlsAlloc();

            if ((TlsIndex_Ctx == TLS_OUT_OF_INDEXES) || (TlsIndex_Ts == TLS_OUT_OF_INDEXES))
                return FALSE;

            // So that Python can figure out where the important
            //   folders are...
            //
            // Between setting the current directory, Py_SetProgramName,
            // and sys.executable, the DLL directory doesn't need to be
            // set in ModuleInitInterp() anymore. For some reason
            // sys.executable still came out to w3wp.exe even when the
            // program name was set to the DLL.
            //

            GetModuleFileName(hInst, ModuleFile, 2047);

            // This var doesn't quite work as a search path generator
            // for Python if the leading '\\?\' remains.
            if (strstr(ModuleFile, "\\\\?\\") == ModuleFile)
                strcpy_s(ModuleFile, &ModuleFile[4]);

            // Get the path and remove the trailing slash
            strncpy_s(ModulePath, ModuleFile, 2047);

            char *Slash = strrchr(ModulePath, '\\');
            if (Slash) *Slash = '\0';

            if (Py_IsInitialized()) {
                Trace(TRP"Warning: Python already initialized!");
                // really don't want this, but don't fail from it.
                ModuleInitSystem();
                return TRUE;
            }

            // set the current directory to the DLLs
            SetCurrentDirectory(ModulePath);

            Trace(TRP"Initializing Python <%s>", ModuleFile);

            // Not sure how to turn compile completely off?
            // Should be an option. Here we do -OO, even if
            // it isn't desirable.
            Py_OptimizeFlag += 2;

            // DEBUG
            // Set PYTHONVERBOSE environment var instead.
            // Py_VerboseFlag += 10;

            InitializeCriticalSectionAndSpinCount(&CsReq, 10000);

            Py_SetProgramName(ModuleFile);

            // python files
            Py_SetPythonHome(ModuleFile);

            // This is a hack. It fixes the problem if site-packages not
            // being included in sys.path because prefix and exec_prefix
            // aren't properly calculated ONLY when the DLL is in the
            // Python home directory. I couldn't figure out why.
            if (!strlen(Py_GetPrefix())) {
                strncpy(Py_GetPrefix(), ModulePath, MAX_PATH);
            }
            if (!strlen(Py_GetExecPrefix())) {
                strncpy(Py_GetExecPrefix(), ModulePath, MAX_PATH);
            }

            ModuleInitSystem();

            Py_InitializeEx(0);


            PyEval_InitThreads(); // GIL Acquired here

            // The reason ReleaseInterpreter() is called below is
            // so that the current thread can be flagged as having
            // an associated state.
            if (!InitializeInterpreters()) {
                ReleaseInterpreter();
                return FALSE;
            }

            ReleaseInterpreter();

        }
            break;

        case DLL_PROCESS_DETACH: {
            if (--Initialized)
                break;

            if (Py_IsInitialized())
                ShutdownInterpreters();
        }
            break;

        case DLL_THREAD_ATTACH: {
            // Trace(TRP"Thread Attached");
            // Don't need to create a thread state until this thread handles
            // a request - IIS WP's create threads on and off for no apparent
            // reason quite frequently. My best guess is it's part of the
            // "pinging" system to make sure everything is still alive.
        }
            break;

        case DLL_THREAD_DETACH: {
            // Trace(TRP"Thread Detached");
            // Don't need the thread state anymore.
            DetachThreadState();
        }
            break;
    }

    return TRUE;
    SAFE_END
    return FALSE;
}

