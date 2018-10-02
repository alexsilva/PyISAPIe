// https://pyisapie.svn.sourceforge.net/svnroot/pyisapie/Tags/1.1.0-rc1/PyISAPIe/Module/ReadWrite.h
// 91 2008-01-10 23:53:59 -0800 (Thu, 10 Jan 2008)
// (C)2008 Phillip Sitbon <phillip@sitbon.net>
//
// Reading & writing file-like objects.
//
#ifndef _Module_ReadWrite_
#define _Module_ReadWrite_

#include "../PyISAPIe.h"

// Read/Write functions
//
PyObject *ReadClient(PyObject *, PyObject *);

PyObject *WriteClient(PyObject *, PyObject *);

// Init functions
//
PyISAPIe_Func(void) ReadWriteInitInterp(Interpreter *const);

PyISAPIe_Func(void) ReadWriteBeginRequest(Context &);

#endif // !_Module_ReadWrite_
