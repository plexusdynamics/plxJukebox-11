


#include "system.h"
#include "VirtualDirectory.h"
#include "FactoryDirectory.h"
#include "Util.h"
#include "utils/URIUtils.h"
#include "Directory.h"
#include "DirectoryCache.h"
#include "SourcesDirectory.h"
#include "storage/MediaManager.h"
#include "File.h"
#include "FileItem.h"
#ifdef _WIN32
#include "WIN32Util.h"
#endif

using namespace XFILE;

namespace XFILE
{

CVirtualDirectory::CVirtualDirectory(void)
{
  m_allowPrompting = true;  // by default, prompting is allowed.
  m_cacheDirectory = DIR_CACHE_ONCE;  // by default, caching is done.
  m_allowNonLocalSources = true;
  m_allowThreads = true;
}

CVirtualDirectory::~CVirtualDirectory(void)
{}

/*!
 \brief Add shares to the virtual directory
 \param VECSOURCES Shares to add
 \sa CMediaSource, VECSOURCES
 */
void CVirtualDirectory::SetSources(const VECSOURCES& vecSources)
{
  m_vecSources = vecSources;
}

/*!
 \brief Retrieve the shares or the content of a directory.
 \param strPath Specifies the path of the directory to retrieve or pass an empty string to get the shares.
 \param items Content of the directory.
 \return Returns \e true, if directory access is successfull.
 \note If \e strPath is an empty string, the share \e items have thumbnails and icons set, else the thumbnails
    and icons have to be set manually.
 */

bool CVirtualDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  return GetDirectory(strPath,items,true);
}
bool CVirtualDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items, bool bUseFileDirectories)
{
  if (!strPath.IsEmpty() && strPath != "files://")
    return CDirectory::GetDirectory(strPath, items, m_strFileMask, bUseFileDirectories, m_allowPrompting, m_cacheDirectory, m_extFileInfo, m_allowThreads);

  // if strPath is blank, clear the list (to avoid parent items showing up)
  if (strPath.IsEmpty())
    items.Clear();

  // return the root listing
  items.SetPath(strPath);

  // grab our shares
  VECSOURCES shares;
  GetSources(shares);
  CSourcesDirectory dir;
  return dir.GetDirectory(shares, items);
}

/*!
 \brief Is the share \e strPath in the virtual directory.
 \param strPath Share to test
 \return Returns \e true, if share is in the virtual directory.
 \note The parameter \e strPath can not be a share with directory. Eg. "iso9660://dir" will return \e false.
    It must be "iso9660://".
 */
bool CVirtualDirectory::IsSource(const CStdString& strPath, VECSOURCES *sources, CStdString *name) const
{
  CStdString strPathCpy = strPath;
  strPathCpy.TrimRight("/");
  strPathCpy.TrimRight("\\");

  // just to make sure there's no mixed slashing in share/default defines
  // ie. f:/video and f:\video was not be recognised as the same directory,
  // resulting in navigation to a lower directory then the share.
  if(URIUtils::IsDOSPath(strPathCpy))
    strPathCpy.Replace("/", "\\");

  VECSOURCES shares;
  if (sources)
    shares = *sources;
  else
    GetSources(shares);
  for (int i = 0; i < (int)shares.size(); ++i)
  {
    const CMediaSource& share = shares.at(i);
    CStdString strShare = share.strPath;
    strShare.TrimRight("/");
    strShare.TrimRight("\\");
    if(URIUtils::IsDOSPath(strShare))
      strShare.Replace("/", "\\");
    if (strShare == strPathCpy)
    {
      if (name)
        *name = share.strName;
      return true;
    }
  }
  return false;
}

/*!
 \brief Is the share \e path in the virtual directory.
 \param path Share to test
 \return Returns \e true, if share is in the virtual directory.
 \note The parameter \e path CAN be a share with directory. Eg. "iso9660://dir" will
       return the same as "iso9660://".
 */
bool CVirtualDirectory::IsInSource(const CStdString &path) const
{
  bool isSourceName;
  VECSOURCES shares;
  GetSources(shares);
  int iShare = CUtil::GetMatchingSource(path, shares, isSourceName);
  if (URIUtils::IsOnDVD(path))
  { // check to see if our share path is still available and of the same type, as it changes during autodetect
    // and GetMatchingSource() is too naive at it's matching
    for (unsigned int i = 0; i < shares.size(); i++)
    {
      CMediaSource &share = shares[i];
      if (URIUtils::IsOnDVD(share.strPath) && share.strPath.Equals(path.Left(share.strPath.GetLength())))
        return true;
    }
    return false;
  }
  // TODO: May need to handle other special cases that GetMatchingSource() fails on
  return (iShare > -1);
}

void CVirtualDirectory::GetSources(VECSOURCES &shares) const
{
  shares = m_vecSources;
  // add our plug n play shares

#ifndef IS_PROFESSIONAL // Laureon: Added: Jukebox: Professional mode shouldn't list removable sources
  if (m_allowNonLocalSources)
    g_mediaManager.GetRemovableDrives(shares);
#endif

#ifdef HAS_DVD_DRIVE
  // and update our dvd share
  for (unsigned int i = 0; i < shares.size(); ++i)
  {
    CMediaSource& share = shares[i];
    if (share.m_iDriveType == CMediaSource::SOURCE_TYPE_DVD)
    {
      if(g_mediaManager.IsAudio(share.strPath))
      {
        share.strStatus = "Audio-CD";
        share.strPath = "cdda://local/";
        share.strDiskUniqueId = "";
      }
      else
      {
        share.strStatus = g_mediaManager.GetDiskLabel(share.strPath);
        share.strDiskUniqueId = g_mediaManager.GetDiskUniqueId(share.strPath);
        if (!share.strPath.length()) // unmounted CD
        {
          if (g_mediaManager.GetDiscPath() == "iso9660://")
            share.strPath = "iso9660://";
          else
            // share is unmounted and not iso9660, discard it
            shares.erase(shares.begin() + i--);
        }
      }
    }
  }
#endif
}
}

