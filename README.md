# PyISAPIe

How to install PyISAPIe:

  First off, this is a little different with Python 2.5 -
  it seems to me that the ISAPI DLL can go anywhere that
  your IWAM_ user has read/execute access to. I chose to
  put mine in the "DLLs" folder of the Python distribution
  so that I can provide the network user with access to
  everything including the Lib folder. The reason this is
  different now is because python25.dll is now in system32.
  If I remember correctly, that wasn't the case with 2.4.
  
  In any case, you won't be able to do "import PyISAPIe"
  because Python no longer imports from .dll files - only
  .pyd works. The problem: Windows complains if you try
  to load a .pyd as an ISAPI extension. Bah! This means
  that the extension is only accessible from within the
  ISAPI process. It doesn't matter much anyway for now since
  it will just complain about not finding a current request
  to work with. You can, however, still access the Http
  package now that I've wrapped the import statement in
  __init__ to ignore import failure.

  2. Place the Http package from Source\PyISAPIe\Python\ somewhere
     in the Python import path. personally, I set up a folder named
     "Web" with a Lib folder where I can put django, the Http
     package, and my Django apps among other things. That way
     I can keep all of my web stuff together. I DO NOT make the
     Web folder my HTTP root!
     I point Python to this folder with the system PYTHONPATH
     environment variable. Remember to reboot (or re-log?)
     after setting it.

Remeber to give the correct permissions to the PyISAPIe.dll
(usually IWAM_computername, or NETWORK SERVICE by default on
IIS6). IIS6 also requires you to create an allowed web
service extension for the DLL.

Running your scripts:

By default, Isapi.py loads the file given to it by the HTTP
server (via the value of Http.Env.SCRIPT_TRANSLATED).
This is the fast way to run, because the memory-cached bytecode
is used instead of reloading every time -- if desired
(and it often is for debugging), just alter the Isapi.py
to reload the module every time. I have yet to observe any
negative effects of reloading the module during concurrent
requests, whether it is done with the imp module, reload,
or execfile. I would appreciate hearing any experiences you
have with this setup.

I've added some other examples, including backwards compatibility
support for v1.0.0 and support for the Django framework in the
Source/Python/Examples folder.

The entire interface now resides in the Http package. It imports
most of the functionality from the PyISAPIe module, including
the following:

  - Header(Header, Status, Length, Close):
    - As of version 1.0.0, you don't have to call this function.
      If you don't, the DefaultHeaders config param is used.

      Header: A string, or list/tuple of strings containing
              headers to be sent. Separators (\r\n) not
              needed. (Clients expect "Header-Name: Value")

      Status: An integer specifying the HTTP status code,
              defaults to 200. The status string is set
              according to HTTP/1.1 specifications.

      Length: Content length of the (only) data being sent.
              This disables chunked transfer encoding, but
              uses keep-alive when available. This can only
              be set once and there must be exactly one write
              with this size.

      Close:  If true, the `Connection: close` header is
              inserted. Disables chunked transfer encoding.

  - Env.<Var>:

      Passes Var as a string to the ISAPI GetServerVariable
      and returns its result (a string). For IIS, see
      http://msdn.microsoft.com/en-us/library/ms524602.aspx
      Note that any leading `\\?\` is NOT removed as it was
      in version 1.0.0. The global variable __target__ corresponding
      to SCRIPT_TRANSLATED has been removed as of v1.0.0.
      
      There is also a special value, KeepAlive, that specifies
      whether or not the current connection will be closed.
      
      As of 1.0.4 the following applies:
      
      Any variable that is not found or not set returns an
      empty string. I found this to be beneficial when setting
      up WSGI.
      
      There are two new bool variables, DataSent and HeadersSent
      for determining if the respective items have been
      transmitted to the client yet.

  - Read([Bytes]):
  
      Read data from the client (i.e. POST data). Excluding
      the parameter returns all available data.
      See help(Http.Read) for more info.

  - Write(Str):
  
      Write a string to the client. Much faster than using the
      print statement.
      See help(Http.Write) for more info.
      
      NOTE: It doesn't do any str() conversion- the caller must
      do it!


Take a look at the Http.Config module for an explanation of the
adjustable parameters.


Extending with your own (C/C++) modules:

I tried to make writing separate modules that use ISAPI as
easy as possible. If you are familiar with programming python
extensions, then you know how wonderfully easy most of it can
be. Interfacing with PyISAPIe is as simple as getting the pointer
to the current thread's request information, contained in the
Context structure (see PyISAPIe.h). The macro PyContext_Current()
does all the work. There is room for extra flags (for now), and
a pointer to the current EXTENSION_CONTROL_BLOCK. Take note that
the Interpreter structure pointed to is defined for ALL requests,
so make sure not to fiddle with it too much (carelessly), since
changes are not local to the current thread. From there, you can
call the ISAPI functions as you normally would.

NOTE: Currently, using PyISAPIe from a different library is broken
because the current context is loaded from a TLS slot. The next
version will (maybe) export a function that can be used to get a
pointer to the context.

Otherwise, it should be pretty easy just to compile your code into
the included project source.

Changes in v1.0.4:
  - Made the move to Subversion. It was relatively painless, and
    I hope to eventually set up a Trac page for the project, which
    means moving away from Sourceforge. For now, it's staying there.

  - Added WSGI support - so far it appears to work well, and it's
    been verified with Django and somewhat tested with Trac. I'd
    like to hear about user experiences with other WSGI-enabled
    apps.

  - Updated to work with Python 2.5 - if anyone needs Python 2.4
    support, I'd be glad to add it. I just figure that 2.5 is
    pretty commonplace at this point.

Changes in v1.0.3:

  - Made the move to Visual Studio 2005 (version 8.0) and made the
    C run-time library statically linked (I'm not sure yet whether
    or not that was a good idea).

  - Added the example 'Advanced' which has a more in-depth regex-to-
    handler mapping system. Also includes an auto-reloading system
    for development (when Config.Debug is True). Note that the URL
    maps are cached, so any changes there still require a restart or
    recycle.

Changes in v1.0.2:
  
  - Fixed a syntax error in Isapi.py caused by a last-minute change.

  - Fixed a string allocation bug in the SCRIPT_TRANSLATED variable
    emulation. Thanks to Bosko Vukov for spotting this one.

Changes in v1.0.1:

  - Added support for an interpreter map, so now you can define
    what interpreter to use based on a server variable. If it's
    not set, then the same interpreter is always used. This is
    much faster and is recommended in most cases.

  - Added support for Django at the request of the first person
    to post in the SF discussion. It was actually pretty easy :)

  - Fixed a bug in Http.Read() thanks to Steve.
  
  - Accessing Env.<Var> when <Var> is not a valid index now returns
    None instead of throwing an exception. If detailed error
    information is needed, use the win32api.GetLastError.

  - Updated some of the buffering and writing logic to speed it
    up a little bit, and also removed a zero-length write that came
    at the end of buffered requests with an empty buffer.

  - Added some example code in Source/Examples, including a much
    better way of running scripts - using regexes and installing
    PyISAPIe as a wildcard handler.

Changes in v1.0.0:

  - Because of the fundamental changes involved, I decided
    that it's about time for the "official 1.0" version.

  - Added support for buffering and chunked transfer encoding,
    as well as keep-alive connections. All of the magic is
    done automatically, as well as sending specified or default
    headers at the first write. Oh yeah, and it's still really
    fast :)

  - Headers are buffered until data is sent, so the Header()
    function can be called multiple times.

  - Restructured the python code, so now everything resides in
    the Http package, including the ISAPI handler module. A
    config module was also added for parameters that are passed
    to the extension.

  - All compiled bytecode is now in optimized form and .pyo files
    are saved for future use.

  - Moved the current context pointer to a TLS slot, which sped
    things up a bit, although temporarily reducing extendability.

Changes in v0.1.2:

  - Fixed all memory leaks, so no more problems with a
    long-running worker process.
  - Removed the HttpTools.CwdHook module because it
    doesn't do anything for opening files (still uses
    the current process' cwd).
  - I was going to add a parameter option to the DLL so
    that the main handler can be specified, but there's
    no easy way to get that parameter from inside the
    ISAPI code. Any suggestions?

Changes in v0.1.1:

  - Fixed some memory leak bugs in the request code.
  - Fixed bugs in Http.Variable()
  - Fixed bugs in Http.Header()
  - Added the HttpTools module and have it importing from ISAPI.py


PyISAPIe has been tested with:

  - Python 2.7.1, Python 2.7.2, Python 2.7.3 and

  - WINDOWS 19 (IIS 7)
  - Server 2012 (IIS 7)

