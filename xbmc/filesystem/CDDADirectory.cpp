

#include "system.h"

#ifdef HAS_DVD_DRIVE

#include "CDDADirectory.h"
#include "music/MusicDatabase.h"
#include "FileItem.h"
#include "File.h"
#include "storage/MediaManager.h"

using namespace XFILE;
using namespace MEDIA_DETECT;

CCDDADirectory::CCDDADirectory(void)
{
}

CCDDADirectory::~CCDDADirectory(void)
{
}


bool CCDDADirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  // Reads the tracks from an audio cd

  if (!g_mediaManager.IsDiscInDrive(strPath))
    return false;

  // Get information for the inserted disc
  CCdInfo* pCdInfo = g_mediaManager.GetCdInfo(strPath);
  if (pCdInfo == NULL)
    return false;

  //  Preload CDDB info
  CMusicDatabase musicdatabase;
  musicdatabase.LookupCDDBInfo();

  // If the disc has no tracks, we are finished here.
  int nTracks = pCdInfo->GetTrackCount();
  if (nTracks <= 0)
    return false;

  // Generate fileitems
  for (int i = 1;i <= nTracks;++i)
  {
    // Skip Datatracks for display,
    // but needed to query cddb
    if (!pCdInfo->IsAudio(i))
      continue;

    // Format standard cdda item label
    CStdString strLabel;
    strLabel.Format("Track %02.2i", i);

    CFileItemPtr pItem(new CFileItem(strLabel));
    pItem->m_bIsFolder = false;
    CStdString path;
    path.Format("cdda://local/%02.2i.cdda", i);
    pItem->SetPath(path);

    struct __stat64 s64;
    if (CFile::Stat(pItem->GetPath(), &s64) == 0)
      pItem->m_dwSize = s64.st_size;

    items.Add(pItem);
  }
  return true;
}

#endif
