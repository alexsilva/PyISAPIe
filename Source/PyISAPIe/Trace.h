// https://pyisapie.svn.sourceforge.net/svnroot/pyisapie/Tags/1.1.0-rc1/PyISAPIe/Trace.h
// 91 2008-01-10 23:53:59 -0800 (Thu, 10 Jan 2008)
// (C)2008 Phillip Sitbon <phillip@sitbon.net>
//
// Debugging without attaching to the ISAPI process.
//
#ifndef _Trace_
#define _Trace_

#include "PyISAPIe.h"

#ifdef TRACE_LOG
#ifndef NO_SEH
// Enable SEH to catch segfaults & Python abort()'s
inline const char *const GetExceptionString(const DWORD &I) {
    switch (I) {
        case EXCEPTION_ACCESS_VIOLATION:
            return "The thread attempted to read from or write to a virtual address for which it does not have the appropriate access (EXCEPTION_ACCESS_VIOLATION)";
            break;
        case EXCEPTION_BREAKPOINT:
            return "A breakpoint was encountered (EXCEPTION_BREAKPOINT)";
            break;
        case EXCEPTION_DATATYPE_MISALIGNMENT:
            return "The thread attempted to read or write data that is misaligned on hardware that does not provide alignment. For example, 16-bit values must be aligned on 2-byte boundaries, 32-bit values on 4-byte boundaries, and so on (EXCEPTION_DATATYPE_MISALIGNMENT)";
            break;
        case EXCEPTION_SINGLE_STEP:
            return "A trace trap or other single-instruction mechanism signaled that one instruction has been executed (EXCEPTION_SINGLE_STEP)";
            break;
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            return "The thread attempted to access an array element that is out of bounds, and the underlying hardware supports bounds checking (EXCEPTION_ARRAY_BOUNDS_EXCEEDED)";
            break;
        case EXCEPTION_FLT_DENORMAL_OPERAND:
            return "One of the operands in a floating-point operation is denormal. A denormal value is one that is too small to represent as a standard floating-point value (EXCEPTION_FLT_DENORMAL_OPERAND)";
            break;
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
            return "The thread attempted to divide a floating-point value by a floating-point divisor of zero (EXCEPTION_FLT_DIVIDE_BY_ZERO)";
            break;
        case EXCEPTION_FLT_INEXACT_RESULT:
            return "The result of a floating-point operation cannot be represented exactly as a decimal fraction (EXCEPTION_FLT_INEXACT_RESULT)";
            break;
        case EXCEPTION_FLT_INVALID_OPERATION:
            return "This exception represents any floating-point exception not included in this list (EXCEPTION_FLT_INVALID_OPERATION)";
            break;
        case EXCEPTION_FLT_OVERFLOW:
            return "The exponent of a floating-point operation is greater than the magnitude allowed by the corresponding type (EXCEPTION_FLT_OVERFLOW)";
            break;
        case EXCEPTION_FLT_STACK_CHECK:
            return "The stack overflowed or underflowed as the result of a floating-point operation (EXCEPTION_FLT_STACK_CHECK)";
            break;
        case EXCEPTION_FLT_UNDERFLOW:
            return "The exponent of a floating-point operation is less than the magnitude allowed by the corresponding type (EXCEPTION_FLT_UNDERFLOW)";
            break;
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            return "The thread attempted to divide an integer value by an integer divisor of zero (EXCEPTION_INT_DIVIDE_BY_ZERO)";
            break;
        case EXCEPTION_INT_OVERFLOW:
            return "The result of an integer operation caused a carry out of the most significant bit of the result (EXCEPTION_INT_OVERFLOW)";
            break;
        case EXCEPTION_PRIV_INSTRUCTION:
            return "The thread attempted to execute an instruction whose operation is not allowed in the current machine mode (EXCEPTION_PRIV_INSTRUCTION)";
            break;
        case EXCEPTION_NONCONTINUABLE_EXCEPTION:
            return "The thread attempted to continue execution after a noncontinuable exception occurred (EXCEPTION_NONCONTINUABLE_EXCEPTION)";
            break;
    }
    return "Unknown Exception";
}

#define SAFE_BEGIN __try {
#define SAFE_END   } __except(EXCEPTION_EXECUTE_HANDLER){\
      Trace(TRP"Failure: %s", GetExceptionString(GetExceptionCode())); }
#else
#define SAFE_BEGIN
#define SAFE_END
#endif

extern "C" void Trace(const char *const Format, ...);
#else
#define SAFE_BEGIN
#define SAFE_END
extern "C" inline void Trace(const char *const Format, ...) {}
#endif


// Trace prefix
#if defined(__INTEL_COMPILER)
#define STR1(x) #x
#define STR2(x) STR1(x)
#define TRP __FILE__"(" STR2(__LINE__) "): "
#else
#define TRP __FUNCTION__": "
#endif

#endif // !_Trace_