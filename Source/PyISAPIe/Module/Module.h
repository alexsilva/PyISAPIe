// https://pyisapie.svn.sourceforge.net/svnroot/pyisapie/Tags/1.1.0-rc1/PyISAPIe/Module/Module.h
// 91 2008-01-10 23:53:59 -0800 (Thu, 10 Jan 2008)
// (C)2008 Phillip Sitbon <phillip@sitbon.net>
//
// The PyISAPIe python module.
//
#ifndef _Module_
#define _Module_

#include "../PyISAPIe.h"

extern "C" {
PyMODINIT_FUNC initPyISAPIe();
}

// Init functions
//
PyISAPIe_Func(void) ModuleInitSystem();

PyISAPIe_Func(void) ModuleInitInterp(Interpreter *const);

PyISAPIe_Func(void) ModuleBeginRequest(Context &);

#endif // !_Module_
