// https://pyisapie.svn.sourceforge.net/svnroot/pyisapie/Tags/1.1.0-rc1/PyISAPIe/ReadWrite.h
// 151 2009-05-20 20:02:46 -0700 (Wed, 20 May 2009)
// (C)2008 Phillip Sitbon <phillip@sitbon.net>
//
// Reading & writing utility functions.
//
#ifndef _ReadWrite_
#define _ReadWrite_

#include "PyISAPIe.h"

PyISAPIe_Func(bool) WriteClient(Context &, DWORD, const char *const);

PyISAPIe_Func(bool) WriteClient(Context &, const char *const, ...);

PyISAPIe_Func(bool) WriteFinishNeeded(Context &);

PyISAPIe_Func(bool) WriteFinish(Context &);

// Returns bytes read or zero on error
PyISAPIe_Func(DWORD) ReadClient(Context &, DWORD, void *const);

// Always returns HSE_STATUS_ERROR
PyISAPIe_Func(DWORD) Fail(Context &, const char *const, ...);

#endif // !_ReadWrite_
