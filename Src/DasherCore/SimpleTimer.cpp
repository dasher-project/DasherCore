
#include "../Common/Common.h"

#include "SimpleTimer.h"

#ifdef _WIN32
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif


CSimpleTimer::CSimpleTimer()
{
#ifdef _WIN32
  struct timeb sTimeBuffer;
#else
  struct timeval sTimeBuffer;
  struct timezone sTimezoneBuffer;
#endif

#ifdef _WIN32
    ftime(&sTimeBuffer);
    m_iStartMs       = sTimeBuffer.millitm;
    m_iStartSecond   = static_cast<int>(sTimeBuffer.time);
#else
    gettimeofday(&sTimeBuffer, &sTimezoneBuffer);
    m_iStartMs       = (int)sTimeBuffer.tv_usec / 1000;
    m_iStartSecond   = (int)sTimeBuffer.tv_sec;
#endif

}

CSimpleTimer::~CSimpleTimer()
{
}

double CSimpleTimer::GetElapsed()
{
#ifdef _WIN32
  struct timeb sTimeBuffer;
#else
  struct timeval sTimeBuffer;
  struct timezone sTimezoneBuffer;
#endif

#ifdef _WIN32
  ftime(&sTimeBuffer);
  int     iEndMs       = sTimeBuffer.millitm;
  int     iEndSecond   = static_cast<int>(sTimeBuffer.time);
#else
  gettimeofday(&sTimeBuffer, &sTimezoneBuffer);
  int     iEndMs       = (int)sTimeBuffer.tv_usec / 1000;
  int     iEndSecond   = (int)sTimeBuffer.tv_sec;
#endif


  return  ((double) iEndMs     / 1000.0 + (double) iEndSecond) - 
          ((double) m_iStartMs / 1000.0 + (double) m_iStartSecond);

}

