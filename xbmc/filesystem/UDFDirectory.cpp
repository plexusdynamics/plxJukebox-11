

#include "UDFDirectory.h"
#include "udf25.h"
#include "Util.h"
#include "URL.h"
#include "FileItem.h"
#include "utils/URIUtils.h"

using namespace XFILE;

CUDFDirectory::CUDFDirectory(void)
{
}

CUDFDirectory::~CUDFDirectory(void)
{
}

bool CUDFDirectory::GetDirectory(const CStdString& strPath,
                                 CFileItemList &items)
{
  CStdString strRoot = strPath;
  URIUtils::AddSlashAtEnd(strRoot);

  CURL url(strPath);

  CStdString strDirectory = url.GetHostName();
  CURL::Decode(strDirectory);

  udf25 udfIsoReader;
  udf_dir_t *dirp = udfIsoReader.OpenDir(strDirectory);

  if (dirp == NULL)
    return false;

  udf_dirent_t *dp = NULL;
  while ((dp = udfIsoReader.ReadDir(dirp)) != NULL)
  {
    if (dp->d_type == DVD_DT_DIR)
    {
      CStdString strDir = (char*)dp->d_name;
      if (strDir != "." && strDir != "..")
      {
        CFileItemPtr pItem(new CFileItem((char*)dp->d_name));
        strDir = strRoot + (char*)dp->d_name;
        URIUtils::AddSlashAtEnd(strDir);
        pItem->SetPath(strDir);
        pItem->m_bIsFolder = true;

        items.Add(pItem);
      }
    }
    else
    {
      CFileItemPtr pItem(new CFileItem((char*)dp->d_name));
      pItem->SetPath(strRoot + (char*)dp->d_name);
      pItem->m_bIsFolder = false;
      pItem->m_dwSize = dp->d_filesize;

      items.Add(pItem);
    }	
  }

  udfIsoReader.CloseDir(dirp);

  return true;
}

bool CUDFDirectory::Exists(const char* strPath)
{
  CFileItemList items;
  if (GetDirectory(strPath,items))
    return true;

  return false;
}
