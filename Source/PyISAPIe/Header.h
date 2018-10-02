// https://pyisapie.svn.sourceforge.net/svnroot/pyisapie/Tags/1.1.0-rc1/PyISAPIe/Header.h
// 119 2008-01-29 03:26:21 -0800 (Tue, 29 Jan 2008)
// (C)2008 Phillip Sitbon <phillip@sitbon.net>
//
// Header management.
//
#ifndef _Header_
#define _Header_

#include "PyISAPIe.h"

PyISAPIe_Func(bool) AddHeaders(Context &, PyObject *);

PyISAPIe_Func(bool) SetDefaultHeaders(Interpreter *const, PyObject *);

PyISAPIe_Func(bool) SetStatus(Context &, const int &);

PyISAPIe_Func(bool) SetLength(Context &, const int &);

PyISAPIe_Func(bool) SetClose(Context &);

PyISAPIe_Func(void) ClearHeaders(Context &);

PyISAPIe_Func(bool) SendHeaders(Context &);

#endif // !_Header_
