

#include "MultiPathFile.h"
#include "MultiPathDirectory.h"
#include "utils/URIUtils.h"
#include "URL.h"

using namespace XFILE;
using namespace std;
using namespace XFILE;

CMultiPathFile::CMultiPathFile(void)
{
}

CMultiPathFile::~CMultiPathFile(void)
{
  Close();
}

bool CMultiPathFile::Open(const CURL& url)
{
  // grab the filename off the url
  CStdString path, fileName;
  URIUtils::Split(url.Get(), path, fileName);
  vector<CStdString> vecPaths;
  if (!CMultiPathDirectory::GetPaths(path, vecPaths))
    return false;

  for (unsigned int i = 0; i < vecPaths.size(); i++)
  {
    CStdString filePath = vecPaths[i];
    filePath = URIUtils::AddFileToFolder(filePath, fileName);
    if (m_file.Open(filePath))
      return true;
  }
  return false;
}

bool CMultiPathFile::Exists(const CURL& url)
{
  // grab the filename off the url
  CStdString path, fileName;
  URIUtils::Split(url.Get(), path, fileName);
  vector<CStdString> vecPaths;
  if (!CMultiPathDirectory::GetPaths(path, vecPaths))
    return false;

  for (unsigned int i = 0; i < vecPaths.size(); i++)
  {
    CStdString filePath = vecPaths[i];
    filePath = URIUtils::AddFileToFolder(filePath, fileName);
    if (CFile::Exists(filePath))
      return true;
  }
  return false;
}

int CMultiPathFile::Stat(const CURL& url, struct __stat64* buffer)
{
  // grab the filename off the url
  CStdString path, fileName;
  URIUtils::Split(url.Get(), path, fileName);
  vector<CStdString> vecPaths;
  if (!CMultiPathDirectory::GetPaths(path, vecPaths))
    return false;

  for (unsigned int i = 0; i < vecPaths.size(); i++)
  {
    CStdString filePath = vecPaths[i];
    filePath = URIUtils::AddFileToFolder(filePath, fileName);
    int ret = CFile::Stat(filePath, buffer);
    if (ret == 0)
      return ret;
  }
  return -1;
}

unsigned int CMultiPathFile::Read(void* lpBuf, int64_t uiBufSize)
{
  return m_file.Read(lpBuf, uiBufSize);
}

int64_t CMultiPathFile::Seek(int64_t iFilePosition, int iWhence /*=SEEK_SET*/)
{
  return m_file.Seek(iFilePosition, iWhence);
}

void CMultiPathFile::Close()
{
  m_file.Close();
}

int64_t CMultiPathFile::GetPosition()
{
  return m_file.GetPosition();
}

int64_t CMultiPathFile::GetLength()
{
  return m_file.GetLength();
}
