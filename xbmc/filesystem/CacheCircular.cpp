

#include "threads/SystemClock.h"
#include "system.h"
#include "utils/log.h"
#include "threads/SingleLock.h"
#include "utils/TimeUtils.h"
#include "CacheCircular.h"

using namespace XFILE;

CCacheCircular::CCacheCircular(size_t front, size_t back)
 : CCacheStrategy()
 , m_beg(0)
 , m_end(0)
 , m_cur(0)
 , m_buf(NULL)
 , m_size(front + back)
 , m_size_back(back)
#ifdef _WIN32
 , m_handle(INVALID_HANDLE_VALUE)
#endif
{
}

CCacheCircular::~CCacheCircular()
{
  Close();
}

int CCacheCircular::Open()
{
#ifdef _WIN32
  m_handle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, m_size, NULL);
  if(m_handle == INVALID_HANDLE_VALUE)
    return CACHE_RC_ERROR;
  m_buf = (uint8_t*)MapViewOfFile(m_handle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
#else
  m_buf = new uint8_t[m_size];
#endif
  if(m_buf == 0)
    return CACHE_RC_ERROR;
  m_beg = 0;
  m_end = 0;
  m_cur = 0;
  return CACHE_RC_OK;
}

void CCacheCircular::Close()
{
#ifdef _WIN32
  UnmapViewOfFile(m_buf);
  CloseHandle(m_handle);
  m_handle = INVALID_HANDLE_VALUE;
#else
  delete[] m_buf;
#endif
  m_buf = NULL;
}

/**
 * Function will write to m_buf at m_end % m_size location
 * it will write at maximum m_size, but it will only write
 * as much it can without wrapping around in the buffer
 *
 * It will always leave m_size_back of the backbuffer intact
 * but if the back buffer is less than that, that space is
 * usable to write.
 *
 * If back buffer is filled to an larger extent than
 * m_size_back, it will allow it to be overwritten
 * until only m_size_back data remains.
 *
 * The following always apply:
 *  * m_end <= m_cur <= m_end
 *  * m_end - m_beg <= m_size
 *
 * Multiple calls may be needed to fill buffer completely.
 */
int CCacheCircular::WriteToCache(const char *buf, size_t len)
{
  CSingleLock lock(m_sync);

  // where are we in the buffer
  size_t pos   = m_end % m_size;
  size_t back  = (size_t)(m_cur - m_beg);
  size_t front = (size_t)(m_end - m_cur);

  size_t limit = m_size - std::min(back, m_size_back) - front;
  size_t wrap  = m_size - pos;

  // limit by max forward size
  if(len > limit)
    len = limit;

  // limit to wrap point
  if(len > wrap)
    len = wrap;

  if(len == 0)
    return 0;

  // write the data
  memcpy(m_buf + pos, buf, len);
  m_end += len;

  // drop history that was overwritten
  if(m_end - m_beg > m_size)
    m_beg = m_end - m_size;

  m_written.Set();

  return len;
}

/**
 * Reads data from cache. Will only read up till
 * the buffer wrap point. So multiple calls
 * may be needed to empty the whole cache
 */
int CCacheCircular::ReadFromCache(char *buf, size_t len)
{
  CSingleLock lock(m_sync);

  size_t pos   = m_cur % m_size;
  size_t front = (size_t)(m_end - m_cur);
  size_t avail = std::min(m_size - pos, front);

  if(avail == 0)
  {
    if(IsEndOfInput())
      return 0;
    else
      return CACHE_RC_WOULD_BLOCK;
  }

  if(len > avail)
    len = avail;

  if(len == 0)
    return 0;

  memcpy(buf, m_buf + pos, len);
  m_cur += len;

  m_space.Set();

  return len;
}

int64_t CCacheCircular::WaitForData(unsigned int minumum, unsigned int millis)
{
  CSingleLock lock(m_sync);
  uint64_t avail = m_end - m_cur;

  if(millis == 0 || IsEndOfInput())
    return avail;

  if(minumum > m_size - m_size_back)
    minumum = m_size - m_size_back;

  XbmcThreads::EndTime endtime(millis);
  while (!IsEndOfInput() && avail < minumum && !endtime.IsTimePast() )
  {
    lock.Leave();
    m_written.WaitMSec(50); // may miss the deadline. shouldn't be a problem.
    lock.Enter();
    avail = m_end - m_cur;
  }

  return avail;
}

int64_t CCacheCircular::Seek(int64_t pos)
{
  CSingleLock lock(m_sync);

  // if seek is a bit over what we have, try to wait a few seconds for the data to be available.
  // we try to avoid a (heavy) seek on the source
  if ((uint64_t)pos >= m_end && (uint64_t)pos < m_end + 100000)
  {
    lock.Leave();
    WaitForData((size_t)(pos - m_cur), 5000);
    lock.Enter();
  }

  if((uint64_t)pos >= m_beg && (uint64_t)pos <= m_end)
  {
    m_cur = pos;
    return pos;
  }

  return CACHE_RC_ERROR;
}

void CCacheCircular::Reset(int64_t pos)
{
  CSingleLock lock(m_sync);
  m_end = pos;
  m_beg = pos;
  m_cur = pos;
}

