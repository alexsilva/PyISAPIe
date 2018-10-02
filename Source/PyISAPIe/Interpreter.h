// https://pyisapie.svn.sourceforge.net/svnroot/pyisapie/Tags/1.1.0-rc1/PyISAPIe/Interpreter.h
// 151 2009-05-20 20:02:46 -0700 (Wed, 20 May 2009)
// (C)2008 Phillip Sitbon <phillip@sitbon.net>
//
// Interpreter & state management functions.
//
#ifndef _Interpreter_
#define _Interpreter_

#include "PyISAPIe.h"

// Sets up main interpreter and interpreter table
PyISAPIe_Func(bool) InitializeInterpreters();

// Return == NULL -> call PyEval_ReleaseLock()
// Return != NULL -> call ReleaseInterpreter()
// [ Be sure to check Initialized == true ]
PyISAPIe_Func(Interpreter *) AcquireInterpreter(register Context &);

// Frees the current thread state
PyISAPIe_Func(void) ReleaseInterpreter();

// Disassociates the current thread with a state and deletes it.
// Has no effect if a state was never created for this thread.
PyISAPIe_Func(void) DetachThreadState();

// Delete interpreters and unload Python
PyISAPIe_Func(void) ShutdownInterpreters();

//
// The following functions interact specifically
// with the python environment.
//

// [Re]Initializes global data & modules
PyISAPIe_Func(bool) InitializeInterpreter(Interpreter *const);
// [Re]Initializes the ISAPI handler module
PyISAPIe_Func(bool) LoadHandler(Interpreter *const);


#endif // !_Interpreter_
