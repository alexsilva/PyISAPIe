// https://pyisapie.svn.sourceforge.net/svnroot/pyisapie/Tags/1.1.0-rc1/PyISAPIe/Module/Doc.h
// 106 2008-01-11 18:46:35 -0800 (Fri, 11 Jan 2008)
// (C)2008 Phillip Sitbon <phillip@sitbon.net>
//
// Documentation strings.
// [For use with Module.cpp only]
//
#ifndef _Module_Doc_
#define _Module_Doc_

#define DOC_STRING(x) static char x[]

namespace Doc {

//-------------------------------------------------------------------
    DOC_STRING(Module) = \
"PyISAPIe built-in server interface.\n\
\n\
  Functions:\n\
    Header -- Specify headers, content length, and more.\n\
    Read   -- Read data from client (i.e. POST data).\n\
    Write  -- Send data to client (faster than print statement).\n\
\n\
  Objects:\n\
    Env    -- Get an attribute via GetServerVariable, for\n\
              example 'Env.QUERY_STRING'.\n\
              Any unrecognized or empty value will result in\n\
              an empty string result.\n\
              There are a few (boolean) attributes as well:\n\
              - KeepAlive: specifies whether the connection will\n\
                           be kept open.\n\
              - HeadersSent: specifies if headers have been sent.\n\
              - DataSent: specifies whether data has been sent.\n";

//-------------------------------------------------------------------
    DOC_STRING(Read) = \
"Read client data.\n\
\n\
  Parameters:\n\
    Bytes -- Number of bytes to read, or zero to read all.\n\
              Set < 0 to determine size of available data.\n\
              Default: Zero.\n\
\n\
  Return value: Data, or size of available data if Bytes < 0\n\
\n\
  If Bytes is zero or is greater than the available data\n\
  size, all data is read.\n";

//-------------------------------------------------------------------
    DOC_STRING(Write) = \
"Write to output.\n\
\n\
  Parameters:\n\
    Data -- string containing data to send.\n\
\n\
  Return value: Always None\n\
\n\
  This function is slightly faster than using print.\n\
  Note that when print is used, softspacing will cause\n\
  the output to be of different lengths - keep this in\n\
  mind when using it with specified content lengths.\n";

//-------------------------------------------------------------------
    DOC_STRING(Header) = \
"Add headers to be sent.\n\
\n\
  Parameters:\n\
    Header -- Either a string (single header) or a list/tuple\n\
              of strings (multiple headers).\n\
              Save yourself a headache and pass only strings!\n\
              Default: Not set\n\
    Status -- Status code identifier (integer). The status\n\
              string is set as per the HTTP/1.1 RFC.\n\
              Default: Not set (200 if never given)\n\
    Length -- Specify the content length. Overrides the use\n\
              of chunked transfer encoding. There must be\n\
              a subsequent write of this many bytes, and\n\
              no more or less.\n\
              Default: Not set\n\
    Close  -- Set to True if you'd like to override the\n\
              keepalive feature.\n\
              Default: Not set\n\
\n\
  Return value: Always None\n\
\n\
  Regardless of keep-alive & HTTP version, headers are not\n\
  sent until either (1) data is sent, or (2) the request\n\
  ends.\n";
}

#endif // !_Module_Doc_