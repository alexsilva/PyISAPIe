// https://pyisapie.svn.sourceforge.net/svnroot/pyisapie/Tags/1.1.0-rc1/PyISAPIe/Trace.cpp
// 151 2009-05-20 20:02:46 -0700 (Wed, 20 May 2009)
// (C)2008 Phillip Sitbon <phillip@sitbon.net>
//
// Debugging without attaching to the ISAPI process.
//
#include "Trace.h"

#ifdef TRACE_LOG
#define TRACE_PATH(x,y) #x""#y

#include <time.h>

/////////////////////////////////////////////////////////////////////
//  Trace()
//
extern "C" void Trace(const char *const Format, ...) {
    static DWORD TlsIndex_Fp = TlsAlloc();
    static int Count = 0;
    static __int64 QpInit;
    static __int64 QpFreq;
    static BOOL _QpInit_UNUSED = QueryPerformanceCounter((LARGE_INTEGER *) &QpInit);
    static BOOL _QpFreq_UNUSED = QueryPerformanceFrequency((LARGE_INTEGER *) &QpFreq);
    __int64 QpNow;
    double QpDiffSec;
    double QpDiffMs;

    QueryPerformanceCounter((LARGE_INTEGER *) &QpNow);
    QpDiffSec = double(QpNow - QpInit) / double(QpFreq);
    QpDiffMs = QpDiffSec;
    QpDiffSec = double(__int64(QpDiffSec));
    QpDiffMs -= QpDiffSec;
    QpDiffMs *= 1000.0;

    FILE *fp = (FILE *) TlsGetValue(TlsIndex_Fp);
    char FormatE[8192];
    va_list Args;

    if (!fp) {
        int MyCount = ++Count;

        snprintf(FormatE, 8191, R"(E:\www\html\PyISAPIe-1.1.0-rc1\Release\trace-%02i-%04d-%04d.txt)",
                 MyCount, GetCurrentProcessId(), GetCurrentThreadId());

        fp = fopen(FormatE, "a+t");
        TlsSetValue(TlsIndex_Fp, (LPVOID) fp);

        double QpFreq_D = (double) QpFreq;
        time_t lt_int = time(NULL);
        tm *lt = localtime(&lt_int);
        char *asc_time = asctime(lt);
        asc_time[strlen(asc_time) - 1] = '\0';
        snprintf(FormatE, 8191, "[%04d-%04d] INIT <%s> PC Frequency = %.0f\n",
                 GetCurrentProcessId(), GetCurrentThreadId(), asc_time, QpFreq_D);
        fwrite(FormatE, 1, strlen(FormatE), fp);
        fflush(fp);
    }

    if (!fp) return;

    va_start(Args, Format);
    snprintf(FormatE, 8191, "[%04d-%04d %04.0f.%08.03f] %s\n", GetCurrentProcessId(),
            GetCurrentThreadId(), QpDiffSec, QpDiffMs, Format);
    vfprintf(fp, FormatE, Args);
    va_end(Args);
    fflush(fp);
}


#endif // TRACE_LOG