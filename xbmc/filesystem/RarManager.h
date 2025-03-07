#if !defined(AFX_RARMANAGER_H__06BA7C2E_3FCA_11D9_8186_0050FC718317__INCLUDED_)
#define AFX_RARMANAGER_H__06BA7C2E_3FCA_11D9_8186_0050FC718317__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "utils/StdString.h"
#include "threads/CriticalSection.h"
#include <map>
#include "UnrarXLib/UnrarX.hpp"
#include "utils/Stopwatch.h"

#include "threads/Thread.h"

class CFileItemList;

#define EXFILE_OVERWRITE 1
#define EXFILE_AUTODELETE 2
#define EXFILE_UNIXPATH 4
#define EXFILE_NOCACHE 8
#define RAR_DEFAULT_CACHE "special://temp/"
#define RAR_DEFAULT_PASSWORD ""

class CFileInfo{
public:
  CFileInfo();
  ~CFileInfo();
  CStdString m_strCachedPath;
  CStdString m_strPathInRar;
  bool  m_bAutoDel;
  int m_iUsed;
  int64_t m_iOffset;

  bool m_bIsCanceled()
  {
    if (watch.IsRunning())
      if (watch.GetElapsedSeconds() < 3)
        return true;

    watch.Stop();
    return false;
  }
  CStopWatch watch;
  int m_iIsSeekable;
};

class CRarManager
{
public:
  CRarManager();
  ~CRarManager();
  bool CacheRarredFile(CStdString& strPathInCache, const CStdString& strRarPath,
                       const CStdString& strPathInRar, BYTE bOptions = EXFILE_AUTODELETE,
                       const CStdString& strDir =RAR_DEFAULT_CACHE, const int64_t iSize=-1);
  bool GetPathInCache(CStdString& strPathInCache, const CStdString& strRarPath,
                      const CStdString& strPathInRar = "");
  bool GetFilesInRar(CFileItemList& vecpItems, const CStdString& strRarPath,
                     bool bMask=true, const CStdString& strPathInRar="");
  CFileInfo* GetFileInRar(const CStdString& strRarPath, const CStdString& strPathInRar);
  bool IsFileInRar(bool& bResult, const CStdString& strRarPath, const CStdString& strPathInRar);
  void ClearCache(bool force=false);
  void ClearCachedFile(const CStdString& strRarPath, const CStdString& strPathInRar);
  void ExtractArchive(const CStdString& strArchive, const CStdString& strPath);
protected:

  bool ListArchive(const CStdString& strRarPath, ArchiveList_struct* &pArchiveList);
  std::map<CStdString, std::pair<ArchiveList_struct*,std::vector<CFileInfo> > > m_ExFiles;
  CCriticalSection m_CritSection;

  int64_t CheckFreeSpace(const CStdString& strDrive);
};

extern CRarManager g_RarManager;
#endif

