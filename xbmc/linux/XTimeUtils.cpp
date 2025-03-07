

#include "system.h"
#include "LinuxTimezone.h"
#include "XTimeUtils.h"
#include "../utils/log.h"
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/times.h>
#include <sched.h>

#ifdef __APPLE__
#include "threads/Atomics.h"
#include <mach/mach_time.h>
#include <CoreVideo/CVHostTime.h>
#endif

#define WIN32_TIME_OFFSET ((unsigned long long)(369 * 365 + 89) * 24 * 3600 * 10000000)

/*
 * A Leap year is any year that is divisible by four, but not by 100 unless also
 * divisible by 400
 */
#define IsLeapYear(y) ((!(y % 4)) ? (((!(y % 400)) && (y % 100)) ? 1 : 0) : 0)

#ifdef _LINUX

void WINAPI Sleep(DWORD dwMilliSeconds)
{
#if _POSIX_PRIORITY_SCHEDULING
  if(dwMilliSeconds == 0)
  {
    sched_yield();
    return;
  }
#endif

  struct timespec req;
  req.tv_sec = dwMilliSeconds / 1000;
  req.tv_nsec = (dwMilliSeconds % 1000) * 1000000;

  // many calls will be interupted. so we keep looping till we're done.
  while ( nanosleep(&req, &req) == -1 && errno == EINTR && (req.tv_nsec > 0 || req.tv_sec > 0))
    ;
}

VOID GetLocalTime(LPSYSTEMTIME sysTime)
{
  const time_t t = time(NULL);
  struct tm now;

  localtime_r(&t, &now);
  sysTime->wYear = now.tm_year + 1900;
  sysTime->wMonth = now.tm_mon + 1;
  sysTime->wDayOfWeek = now.tm_wday;
  sysTime->wDay = now.tm_mday;
  sysTime->wHour = now.tm_hour;
  sysTime->wMinute = now.tm_min;
  sysTime->wSecond = now.tm_sec;
  sysTime->wMilliseconds = 0;
  // NOTE: localtime_r() is not required to set this, but we Assume that it's set here.
  g_timezone.m_IsDST = now.tm_isdst;
}

BOOL FileTimeToLocalFileTime(const FILETIME* lpFileTime, LPFILETIME lpLocalFileTime)
{
  // TODO: FileTimeToLocalTime not implemented
  *lpLocalFileTime = *lpFileTime;
  return true;
}

BOOL   SystemTimeToFileTime(const SYSTEMTIME* lpSystemTime,  LPFILETIME lpFileTime)
{
  static const int dayoffset[12] = {0, 31, 59, 90, 120, 151, 182, 212, 243, 273, 304, 334};
#ifdef __APPLE__
  static long timegm_lock = 0;
#endif

  struct tm sysTime = {};
  sysTime.tm_year = lpSystemTime->wYear - 1900;
  sysTime.tm_mon = lpSystemTime->wMonth - 1;
  sysTime.tm_wday = lpSystemTime->wDayOfWeek;
  sysTime.tm_mday = lpSystemTime->wDay;
  sysTime.tm_hour = lpSystemTime->wHour;
  sysTime.tm_min = lpSystemTime->wMinute;
  sysTime.tm_sec = lpSystemTime->wSecond;
  sysTime.tm_yday = dayoffset[sysTime.tm_mon] + (sysTime.tm_mday - 1);
  sysTime.tm_isdst = g_timezone.m_IsDST;

  // If this is a leap year, and we're past the 28th of Feb, increment tm_yday.
  if (IsLeapYear(lpSystemTime->wYear) && (sysTime.tm_yday > 58))
    sysTime.tm_yday++;

#ifdef __APPLE__
  CAtomicSpinLock lock(timegm_lock);
#endif
  time_t t = timegm(&sysTime);

  LARGE_INTEGER result;
  result.QuadPart = (long long) t * 10000000 + (long long) lpSystemTime->wMilliseconds * 10000;
  result.QuadPart += WIN32_TIME_OFFSET;

  lpFileTime->dwLowDateTime = result.u.LowPart;
  lpFileTime->dwHighDateTime = result.u.HighPart;

  return 1;
}

LONG   CompareFileTime(const FILETIME* lpFileTime1, const FILETIME* lpFileTime2)
{
  ULARGE_INTEGER t1;
  t1.u.LowPart = lpFileTime1->dwLowDateTime;
  t1.u.HighPart = lpFileTime1->dwHighDateTime;

  ULARGE_INTEGER t2;
  t2.u.LowPart = lpFileTime2->dwLowDateTime;
  t2.u.HighPart = lpFileTime2->dwHighDateTime;

  if (t1.QuadPart == t2.QuadPart)
     return 0;
  else if (t1.QuadPart < t2.QuadPart)
     return -1;
  else
     return 1;
}

BOOL   FileTimeToSystemTime( const FILETIME* lpFileTime, LPSYSTEMTIME lpSystemTime)
{
  LARGE_INTEGER fileTime;
  fileTime.u.LowPart = lpFileTime->dwLowDateTime;
  fileTime.u.HighPart = lpFileTime->dwHighDateTime;

  fileTime.QuadPart -= WIN32_TIME_OFFSET;
  fileTime.QuadPart /= 10000; /* to milliseconds */
  lpSystemTime->wMilliseconds = fileTime.QuadPart % 1000;
  fileTime.QuadPart /= 1000; /* to seconds */

  time_t ft = fileTime.QuadPart;

  struct tm tm_ft;
  gmtime_r(&ft,&tm_ft);

  lpSystemTime->wYear = tm_ft.tm_year + 1900;
  lpSystemTime->wMonth = tm_ft.tm_mon + 1;
  lpSystemTime->wDayOfWeek = tm_ft.tm_wday;
  lpSystemTime->wDay = tm_ft.tm_mday;
  lpSystemTime->wHour = tm_ft.tm_hour;
  lpSystemTime->wMinute = tm_ft.tm_min;
  lpSystemTime->wSecond = tm_ft.tm_sec;

  return 1;
}

BOOL   LocalFileTimeToFileTime( const FILETIME* lpLocalFileTime, LPFILETIME lpFileTime)
{
  ULARGE_INTEGER l;
  l.u.LowPart = lpLocalFileTime->dwLowDateTime;
  l.u.HighPart = lpLocalFileTime->dwHighDateTime;

  l.QuadPart += (unsigned long long) timezone * 10000000;

  lpFileTime->dwLowDateTime = l.u.LowPart;
  lpFileTime->dwHighDateTime = l.u.HighPart;

  return 1;
}

BOOL  FileTimeToTimeT(const FILETIME* lpLocalFileTime, time_t *pTimeT) {

  if (lpLocalFileTime == NULL || pTimeT == NULL)
  return false;

  ULARGE_INTEGER fileTime;
  fileTime.u.LowPart  = lpLocalFileTime->dwLowDateTime;
  fileTime.u.HighPart = lpLocalFileTime->dwHighDateTime;

  fileTime.QuadPart -= WIN32_TIME_OFFSET;
  fileTime.QuadPart /= 10000; /* to milliseconds */
  fileTime.QuadPart /= 1000; /* to seconds */

  time_t ft = fileTime.QuadPart;

  struct tm tm_ft;
  localtime_r(&ft,&tm_ft);

  *pTimeT = mktime(&tm_ft);
  return true;
}

BOOL  TimeTToFileTime(time_t timeT, FILETIME* lpLocalFileTime) {

  if (lpLocalFileTime == NULL)
  return false;

  ULARGE_INTEGER result;
  result.QuadPart = (unsigned long long) timeT * 10000000;
  result.QuadPart += WIN32_TIME_OFFSET;

  lpLocalFileTime->dwLowDateTime  = result.u.LowPart;
  lpLocalFileTime->dwHighDateTime = result.u.HighPart;

  return true;
}

void GetSystemTimeAsFileTime(LPFILETIME lpSystemTimeAsFileTime)
{
  TimeTToFileTime(time(NULL), lpSystemTimeAsFileTime);
}

#endif

