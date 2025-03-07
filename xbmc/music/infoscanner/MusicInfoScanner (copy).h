#pragma once

#include "threads/Thread.h"
#include "music/MusicDatabase.h"
#include "MusicAlbumInfo.h"

class CAlbum;
class CArtist;

namespace MUSIC_INFO
{
enum SCAN_STATE { PREPARING = 0, REMOVING_OLD, CLEANING_UP_DATABASE, READING_MUSIC_INFO, DOWNLOADING_ALBUM_INFO, DOWNLOADING_ARTIST_INFO, COMPRESSING_DATABASE, WRITING_CHANGES };

class IMusicInfoScannerObserver
{
public:
  virtual ~IMusicInfoScannerObserver() {}
  virtual void OnStateChanged(SCAN_STATE state) = 0;
  virtual void OnDirectoryChanged(const CStdString& strDirectory) = 0;
  virtual void OnDirectoryScanned(const CStdString& strDirectory) = 0;
  virtual void OnSetProgress(int currentItem, int itemCount)=0;
  virtual void OnFinished() = 0;
};

class CMusicInfoScanner : CThread, public IRunnable
{
public:
  CMusicInfoScanner();
  virtual ~CMusicInfoScanner();

  void Start(const CStdString& strDirectory);
  void FetchAlbumInfo(const CStdString& strDirectory);
  void FetchArtistInfo(const CStdString& strDirectory);
  bool IsScanning();
  void Stop();
  void SetObserver(IMusicInfoScannerObserver* pObserver);

  static void CheckForVariousArtists(VECSONGS &songs);
  static bool HasSingleAlbum(const VECSONGS &songs, CStdString &album, CStdString &artist);

  bool DownloadAlbumInfo(const CStdString& strPath, const CStdString& strArtist, const CStdString& strAlbum, bool& bCanceled, MUSIC_GRABBER::CMusicAlbumInfo& album, CGUIDialogProgress* pDialog=NULL);
  bool DownloadArtistInfo(const CStdString& strPath, const CStdString& strArtist, bool& bCanceled, CGUIDialogProgress* pDialog=NULL);
protected:
  virtual void Process();
  int RetrieveMusicInfo(CFileItemList& items, const CStdString& strDirectory);
  void UpdateFolderThumb(const VECSONGS &songs, const CStdString &folderPath);
  int GetPathHash(const CFileItemList &items, CStdString &hash);
  void GetAlbumArtwork(long id, const CAlbum &artist);
  void GetArtistArtwork(long id, const CStdString &artistName, const CArtist *artist = NULL);

  bool DoScan(const CStdString& strDirectory);

  virtual void Run();
  int CountFiles(const CFileItemList& items, bool recursive);
  int CountFilesRecursively(const CStdString& strPath);

protected:
  IMusicInfoScannerObserver* m_pObserver;
  int m_currentItem;
  int m_itemCount;
  bool m_bRunning;
  bool m_bCanInterrupt;
  bool m_needsCleanup;
  int m_scanType; // 0 - load from files, 1 - albums, 2 - artists
  CMusicDatabase m_musicDatabase;

  std::set<CStdString> m_pathsToScan;
  std::set<CAlbum> m_albumsToScan;
  std::set<CArtist> m_artistsToScan;
  std::set<CStdString> m_pathsToCount;
  std::vector<long> m_artistsScanned;
  std::vector<long> m_albumsScanned;
};
}
