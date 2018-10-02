// https://pyisapie.svn.sourceforge.net/svnroot/pyisapie/Tags/1.1.0-rc1/PyISAPIe/Module/Env.h
// 91 2008-01-10 23:53:59 -0800 (Thu, 10 Jan 2008)
// (C)2008 Phillip Sitbon <phillip@sitbon.net>
//
// A "smart" interface to GetServerVariable.
//
#ifndef _Module_Env_
#define _Module_Env_

#include "../PyISAPIe.h"

// Init functions
//
PyISAPIe_Func(void) EnvInitSystem();

PyISAPIe_Func(void) EnvInitModule(PyObject *);

#endif // !_Module_Env_
