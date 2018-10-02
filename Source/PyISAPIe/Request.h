// https://pyisapie.svn.sourceforge.net/svnroot/pyisapie/Tags/1.1.0-rc1/PyISAPIe/Request.h
// 91 2008-01-10 23:53:59 -0800 (Thu, 10 Jan 2008)
// (C)2008 Phillip Sitbon <phillip@sitbon.net>
//
// Request handling related functions.
//
#ifndef _Request_
#define _Request_

#include "PyISAPIe.h"

// Handle a request based on the given context.
PyISAPIe_Func(DWORD) HandleRequest(Context &);

#endif // !_Request_
