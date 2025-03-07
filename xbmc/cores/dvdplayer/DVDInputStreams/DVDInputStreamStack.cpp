

#include "DVDInputStreamStack.h"
#include "FileItem.h"
#include "filesystem/File.h"
#include "filesystem/StackDirectory.h"
#include "utils/log.h"

#include <limits.h>

using namespace XFILE;
using namespace boost;
using namespace std;

CDVDInputStreamStack::CDVDInputStreamStack() : CDVDInputStream(DVDSTREAM_TYPE_FILE)
{
  m_eof = true;
  m_pos = 0;
}

CDVDInputStreamStack::~CDVDInputStreamStack()
{
  Close();
}

bool CDVDInputStreamStack::IsEOF()
{
  return m_eof;
}

bool CDVDInputStreamStack::Open(const char* path, const std::string& content)
{
  if (!CDVDInputStream::Open(path, content))
    return false;

  CStackDirectory dir;
  CFileItemList   items;

  if(!dir.GetDirectory(path, items))
  {
    CLog::Log(LOGERROR, "CDVDInputStreamStack::Open - failed to get list of stacked items");
    return false;
  }

  m_length = 0;
  m_eof    = false;

  for(int index = 0; index < items.Size(); index++)
  {
    TFile file(new CFile());

    if (!file->Open(items[index]->GetPath(), READ_TRUNCATED))
    {
      CLog::Log(LOGERROR, "CDVDInputStreamStack::Open - failed to open stack part '%s' - skipping", items[index]->GetPath().c_str());
      continue;
    }
    TSeg segment;
    segment.file   = file;
    segment.length = file->GetLength();

    if(segment.length <= 0)
    {
      CLog::Log(LOGERROR, "CDVDInputStreamStack::Open - failed to get file length for '%s' - skipping", items[index]->GetPath().c_str());
      continue;
    }

    m_length += segment.length;

    m_files.push_back(segment);
  }

  if(m_files.size() == 0)
    return false;

  m_file = m_files[0].file;
  m_eof  = false;

  return true;
}

// close file and reset everyting
void CDVDInputStreamStack::Close()
{
  CDVDInputStream::Close();
  m_files.clear();
  m_file.reset();
  m_eof = true;
}

int CDVDInputStreamStack::Read(BYTE* buf, int buf_size)
{
  if(m_file == NULL || m_eof)
    return 0;

  unsigned int ret = m_file->Read(buf, buf_size);

  if(ret > INT_MAX)
    return -1;

  if(ret == 0)
  {
    m_eof = true;
    if(Seek(m_pos, SEEK_SET) < 0)
    {
      CLog::Log(LOGERROR, "CDVDInputStreamStack::Read - failed to seek into next file");
      m_eof  = true;
      m_file.reset();
      return -1;
    }
  }

  m_pos += ret;

  return (int)ret;
}

__int64 CDVDInputStreamStack::Seek(__int64 offset, int whence)
{
  __int64 pos, len;

  if     (whence == SEEK_SET)
    pos = offset;
  else if(whence == SEEK_CUR)
    pos = offset + m_pos;
  else if(whence == SEEK_END)
    pos = offset + m_length;
  else
    return -1;

  len = 0;
  for(TSegVec::iterator it = m_files.begin(); it != m_files.end(); it++)
  {
    if(len + it->length > pos)
    {
      TFile   file     = it->file;
      __int64 file_pos = pos - len;
      if(file->GetPosition() != file_pos)
      {
        if(file->Seek(file_pos, SEEK_SET) < 0)
          return false;
      }

      m_file = file;
      m_pos  = pos;
      m_eof  = false;
      return pos;
    }
    len += it->length;
  }

  return -1;
}

__int64 CDVDInputStreamStack::GetLength()
{
  return m_length;
}


