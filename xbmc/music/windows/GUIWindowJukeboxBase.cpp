
#include "threads/SystemClock.h"
#include "system.h"
#include "GUIUserMessages.h"
#include "GUIWindowJukeboxBase.h"
#include "music/dialogs/GUIDialogMusicInfo.h"
#include "filesystem/ZipManager.h"
#include "filesystem/PFCManager.h" // Laureon: Added: Filesystem
#ifdef HAS_FILESYSTEM_DAAP
#include "filesystem/DAAPDirectory.h"
#endif
#include "playlists/PlayListFactory.h"
#include "Util.h"
#include "playlists/PlayListM3U.h"
#include "Application.h"
#include "PlayListPlayer.h"
#include "filesystem/DirectoryCache.h"
#ifdef HAS_CDDA_RIPPER
#include "cdrip/CDDARipper.h"
#endif
#include "GUIPassword.h"
#include "music/dialogs/GUIDialogMusicScan.h"
#include "dialogs/GUIDialogMediaSource.h"
#include "PartyModeManager.h"
#include "GUIInfoManager.h"
#include "filesystem/MusicDatabaseDirectory.h"
#include "music/dialogs/GUIDialogSongInfo.h"
#include "dialogs/GUIDialogSmartPlaylistEditor.h"
#include "music/LastFmManager.h"
#include "music/tags/MusicInfoTag.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogKeyboard.h"
#include "dialogs/GUIDialogProgress.h"
#include "FileItem.h"
#include "filesystem/File.h"
#include "storage/MediaManager.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "guilib/LocalizeStrings.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "video/VideoInfoTag.h"
#include "ThumbnailCache.h"
#include "../../guilib/GUIListContainer.h"

using namespace std;
using namespace XFILE;
using namespace MUSICDATABASEDIRECTORY;
using namespace PLAYLIST;
using namespace MUSIC_GRABBER;
using namespace MUSIC_INFO;

#define CONTROL_BTNVIEWASICONS  2
#define CONTROL_BTNSORTBY       3
#define CONTROL_BTNSORTASC      4
#define CONTROL_BTNTYPE         5

#define CONTROL_JUKEBOXLIST				666

CGUIWindowJukeboxBase::CGUIWindowJukeboxBase(int id, const CStdString &xmlFile) :
    CGUIMediaWindow(id, xmlFile) {

}

CGUIWindowJukeboxBase::~CGUIWindowJukeboxBase() {
}

bool CGUIWindowJukeboxBase::OnBack(int actionID) {
  CGUIDialogMusicScan *musicScan = (CGUIDialogMusicScan *) g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
  if (musicScan && !musicScan->IsDialogRunning())
  {
    CUtil::ThumbCacheClear();
    CUtil::RemoveTempFiles();
  }
  return CGUIMediaWindow::OnBack(actionID);
}

/*!
 \brief Handle messages on window.
 \param message GUI Message that can be reacted on.
 \return if a message can't be processed, return \e false

 On these messages this class reacts.\n
 When retrieving...
 - #GUI_MSG_WINDOW_DEINIT\n
 ...the last focused control is saved to m_iLastControl.
 - #GUI_MSG_WINDOW_INIT\n
 ...the musicdatabase is opend and the music extensions and shares are set.
 The last focused control is set.
 - #GUI_MSG_CLICKED\n
 ... the base class reacts on the following controls:\n
 Buttons:\n
 - #CONTROL_BTNVIEWASICONS - switch between list, thumb and with large items
 - #CONTROL_BTNTYPE - switch between music windows
 - #CONTROL_BTNSEARCH - Search for items\n
 Other Controls:
 - The container controls\n
 Have the following actions in message them clicking on them.
 - #ACTION_QUEUE_ITEM - add selected item to playlist
 - #ACTION_SHOW_INFO - retrieve album info from the internet
 - #ACTION_SELECT_ITEM - Item has been selected. Overwrite OnClick() to react on it
 */
bool CGUIWindowJukeboxBase::OnMessage(CGUIMessage& message) {
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_DEINIT: {
      m_musicdatabase.Close();
    }
    break;

    case GUI_MSG_WINDOW_INIT: {
      m_dlgProgress = (CGUIDialogProgress*) g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
//#ifdef IS_PROFESSIONAL // Laureon: Added: InsertCoin Dialog
//      m_dlgInsertCoin = (CGUIDialogKaiToast *) g_windowManager.GetWindow(WINDOW_DIALOG_KAI_TOAST);
//#endif

      m_musicdatabase.Open();

      if (!CGUIMediaWindow::OnMessage(message)) return false;

      // save current window, unless the current window is the music playlist window
      if (GetID() != WINDOW_MUSIC_PLAYLIST && g_settings.m_iMyMusicStartWindow != GetID())
      {
        g_settings.m_iMyMusicStartWindow = GetID();
        g_settings.Save();
      }
      return true;
    }
    break;

      // update the display
    case GUI_MSG_SCAN_FINISHED:
    case GUI_MSG_REFRESH_THUMBS: {
      Update(m_vecItems->GetPath());
    }
    break;

    case GUI_MSG_PLAYLIST_CHANGED:
      SavePlayList("special://masterprofile/playlist.m3u");
    break;

    case GUI_MSG_CLICKED: {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNTYPE)
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_BTNTYPE);
        g_windowManager.SendMessage(msg);

        int nWindow = WINDOW_MUSIC_FILES + msg.GetParam1();

        if (nWindow == GetID()) return true;

        g_settings.m_iMyMusicStartWindow = nWindow;
        g_settings.Save();
        g_windowManager.ChangeActiveWindow(nWindow);

        CGUIMessage msg2(GUI_MSG_SETFOCUS, g_settings.m_iMyMusicStartWindow, CONTROL_BTNTYPE);
        g_windowManager.SendMessage(msg2);

        return true;
      }
#ifdef IS_JUKEBOX // Laureon: Added: Jukebox listing mode
      else if (iControl == CONTROL_JUKEBOXLIST || m_viewControl.HasControl(iControl))
      {
        //if (m_viewControl.HasControl(iControl)) { // list/thumb control
        //int iItem = m_viewControl.GetSelectedItem();
        CFileItemPtr pItem;
        CGUIListContainer *control = (CGUIListContainer *) GetControl(iControl);

        if (iControl == CONTROL_JUKEBOXLIST)
        {
          if (control && control->IsContainer())
            pItem = m_vecTracks->Get(control->GetSelectedItem());
          else
            return false;
        }
        else
        {
          pItem = m_vecItems->Get(control->GetSelectedItem()); // Laureon: TODO: This is attached to m_vecItems... Change that...
        }

        int iAction = message.GetParam1();

        // iItem is checked for validity inside these routines
        if (iAction == ACTION_QUEUE_ITEM || iAction == ACTION_MOUSE_MIDDLE_CLICK)
        {
          OnQueueItem(pItem);
        }
        else if (iAction == ACTION_SHOW_INFO)
        {
          OnInfo(pItem);
        }
        else if (iAction == ACTION_DELETE_ITEM)
        {
          // is delete allowed?
          // must be at the playlists directory
          //     if (m_vecItems->m_strPath.Equals("special://musicplaylists/"))
          //       OnDeleteItem(pItem);

          //     // or be at the files window and have file deletion enabled
          //     else if (GetID() == WINDOW_MUSIC_FILES && g_guiSettings.GetBool("filelists.allowfiledeletion")) {
          //       OnDeleteItem(pItem);
          //} else 
          return false;
        }
        // use play button to add folders of items to temp playlist
        else if (iAction == ACTION_PLAYER_PLAY)
        {
          // if playback is paused or playback speed != 1, return
          if (g_application.IsPlayingAudio())
          {
            if (g_application.m_pPlayer->IsPaused()) return false;
            if (g_application.GetPlaySpeed() != 1) return false;
          }

          // not playing audio, or playback speed == 1
          PlayItem(pItem);

          return true;
        }
      }
#else      
      else if (m_viewControl.HasControl(iControl))  // list/thumb control
      {
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();

        // iItem is checked for validity inside these routines
        if (iAction == ACTION_QUEUE_ITEM || iAction == ACTION_MOUSE_MIDDLE_CLICK)
        {
          OnQueueItem(iItem);
        }
        else if (iAction == ACTION_SHOW_INFO)
        {
          OnInfo(iItem);
        }
        else if (iAction == ACTION_DELETE_ITEM)
        {
          // is delete allowed?
          // must be at the playlists directory
          if (m_vecItems->GetPath().Equals("special://musicplaylists/"))
          OnDeleteItem(iItem);

          // or be at the files window and have file deletion enabled
          else if (GetID() == WINDOW_MUSIC_FILES &&
              g_guiSettings.GetBool("filelists.allowfiledeletion"))
          {
            OnDeleteItem(iItem);
          }

          else
          return false;
        }
        // use play button to add folders of items to temp playlist
        else if (iAction == ACTION_PLAYER_PLAY)
        {
          // if playback is paused or playback speed != 1, return
          if (g_application.IsPlayingAudio())
          {
            if (g_application.m_pPlayer->IsPaused())
            return false;
            if (g_application.GetPlaySpeed() != 1)
            return false;
          }

          // not playing audio, or playback speed == 1
          PlayItem(iItem);

          return true;
        }
      }
#endif
    }
  }
  return CGUIMediaWindow::OnMessage(message);
}

void CGUIWindowJukeboxBase::OnInfoAll(CFileItemPtr pItem, bool bCurrent) {
  CGUIDialogMusicScan* musicScan = (CGUIDialogMusicScan *) g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
  CMusicDatabaseDirectory dir;
  CStdString strPath = m_vecItems->GetPath();
  if (bCurrent) strPath = pItem->GetPath();

  if (dir.HasAlbumInfo(pItem->GetPath()))
    musicScan->StartAlbumScan(strPath);
  else
    musicScan->StartArtistScan(strPath);
}

/// \brief Retrieves music info for albums from allmusic.com and displays them in CGUIDialogMusicInfo
/// \param iItem Item in list/thumb control
//void CGUIWindowJukeboxBase::OnInfo(int iItem, bool bShowInfo)
//{
//  if ( iItem < 0 || iItem >= m_vecItems->Size() )
//    return;
//
//  CFileItemPtr item = m_vecItems->Get(iItem);
//
//
//  OnInfo(item.get(), bShowInfo);
//}

void CGUIWindowJukeboxBase::OnInfo(CFileItemPtr pItem, bool bShowInfo) {
  if (pItem->IsVideoDb())
  { // music video
    CFileItemPtr itempointer(pItem);
    OnContextButton(itempointer, CONTEXT_BUTTON_INFO);
    return;
  }

  if ((pItem->IsMusicDb() && !pItem->HasMusicInfoTag()) || pItem->IsParentFolder() || URIUtils::IsSpecial(pItem->GetPath()) || pItem->GetPath().Left(14).Equals("musicsearch://")) return; // nothing to do

  if (!pItem->m_bIsFolder)
  { // song lookup
    ShowSongInfo(pItem);
    return;
  }

  CStdString strPath = pItem->GetPath();

  // Try to find an album to lookup from the current item
  CAlbum album;
  CArtist artist;
  bool foundAlbum = false;

  album.idAlbum = -1;

  // we have a folder
  if (pItem->IsMusicDb())
  {
    CQueryParams params;
    CDirectoryNode::GetDatabaseInfo(pItem->GetPath(), params);
    if (params.GetAlbumId() == -1)
    { // artist lookup
      artist.idArtist = params.GetArtistId();
      artist.strArtist = pItem->GetMusicInfoTag()->GetArtist();
    }
    else
    { // album lookup
      album.idAlbum = params.GetAlbumId();
      album.strAlbum = pItem->GetMusicInfoTag()->GetAlbum();
      album.strArtist = pItem->GetMusicInfoTag()->GetArtist();

      // we're going to need it's path as well (we assume that there's only one) - this is for
      // assigning thumbs to folders, and obtaining the local folder.jpg
      m_musicdatabase.GetAlbumPath(album.idAlbum, strPath);
    }
  }

  //else
  //{ // from filemode, so find the albums in the folder
  //  CFileItemList items;
  //  GetDirectory(strPath, items);

  //  // show dialog box indicating we're searching the album name
  //  if (m_dlgProgress && bShowInfo)
  //  {
  //    m_dlgProgress->SetHeading(185);
  //    m_dlgProgress->SetLine(0, 501);
  //    m_dlgProgress->SetLine(1, "");
  //    m_dlgProgress->SetLine(2, "");
  //    m_dlgProgress->StartModal();
  //    m_dlgProgress->Progress();
  //    if (m_dlgProgress->IsCanceled())
  //      return;
  //  }
  //  // check the first song we find in the folder, and grab it's album info
  //  for (int i = 0; i < items.Size() && !foundAlbum; i++)
  //  {
  //    CFileItemPtr pItem = items[i];
  //    pItem->LoadMusicTag();
  //    if (pItem->HasMusicInfoTag() && pItem->GetMusicInfoTag()->Loaded() &&
  //       !pItem->GetMusicInfoTag()->GetAlbum().IsEmpty())
  //    {
  //      // great, have a song - use it.
  //      CSong song(*pItem->GetMusicInfoTag());
  //      // this function won't be needed if/when the tag has idSong information
  //      if (!m_musicdatabase.GetAlbumFromSong(song, album))
  //      { // album isn't in the database - construct it from the tag info we have
  //        CMusicInfoTag *tag = pItem->GetMusicInfoTag();
  //        album.strAlbum = tag->GetAlbum();
  //        album.strArtist = tag->GetAlbumArtist().IsEmpty() ? tag->GetArtist() : tag->GetAlbumArtist();
  //        album.idAlbum = -1; // the -1 indicates it's not in the database
  //      }
  //      foundAlbum = true;
  //    }
  //  }
  //  if (!foundAlbum)
  //  {
  //    CLog::Log(LOGINFO, "%s called on a folder containing no songs with tag info - nothing can be done", __FUNCTION__);
  //    if (m_dlgProgress && bShowInfo)
  //      m_dlgProgress->Close();
  //    return;
  //  }

  //  if (m_dlgProgress && bShowInfo)
  //    m_dlgProgress->Close();
  //}

  if (album.idAlbum == -1 && foundAlbum == false)
    ShowArtistInfo(artist, pItem->GetPath(), false, bShowInfo);
  else
    ShowAlbumInfo(album, strPath, false, bShowInfo);
}

void CGUIWindowJukeboxBase::OnManualAlbumInfo() {
  CAlbum album;
  album.idAlbum = -1; // not in the db
  if (!CGUIDialogKeyboard::ShowAndGetInput(album.strAlbum, g_localizeStrings.Get(16011), false)) return;

  CStdString strNewArtist = "";
  if (!CGUIDialogKeyboard::ShowAndGetInput(album.strArtist, g_localizeStrings.Get(16025), false)) return;

  ShowAlbumInfo(album, "", true);
}

void CGUIWindowJukeboxBase::ShowArtistInfo(const CArtist& artist, const CStdString& path, bool bRefresh, bool bShowInfo) {
  bool saveDb = artist.idArtist != -1;
  if (!g_settings.GetCurrentProfile().canWriteDatabases() && !g_passwordManager.bMasterUser) saveDb = false;

  // check cache
  CArtist artistInfo;
  if (!bRefresh && m_musicdatabase.GetArtistInfo(artist.idArtist, artistInfo))
  {
    if (!bShowInfo) return;

    CGUIDialogMusicInfo *pDlgArtistInfo = (CGUIDialogMusicInfo*) g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_INFO);
    if (pDlgArtistInfo)
    {
      pDlgArtistInfo->SetArtist(artistInfo, path);

      if (bShowInfo)
        pDlgArtistInfo->DoModal();
      else
        pDlgArtistInfo->RefreshThumb();  // downloads the thumb if we don't already have one

      if (!pDlgArtistInfo->NeedRefresh())
      {
        if (pDlgArtistInfo->HasUpdatedThumb()) Update(m_vecItems->GetPath());

        return;
      }
      bRefresh = true;
      m_musicdatabase.DeleteArtistInfo(artistInfo.idArtist);
    }
  }

  // If we are scanning for music info in the background,
  // other writing access to the database is prohibited.
  CGUIDialogMusicScan* dlgMusicScan = (CGUIDialogMusicScan*) g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
  if (dlgMusicScan->IsDialogRunning())
  {
    CGUIDialogOK::ShowAndGetInput(189, 14057, 0, 0);
    return;
  }

  CMusicArtistInfo info;
  if (FindArtistInfo(artist.strArtist, info, bShowInfo ? (bRefresh ? SELECTION_FORCED : SELECTION_ALLOWED) : SELECTION_AUTO))
  {
    // download the album info
    if (info.Loaded())
    {
      if (saveDb)
      {
        // save to database
        m_musicdatabase.SetArtistInfo(artist.idArtist, info.GetArtist());
      }
      if (m_dlgProgress && bShowInfo) m_dlgProgress->Close();

      // ok, show album info
      CGUIDialogMusicInfo *pDlgArtistInfo = (CGUIDialogMusicInfo*) g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_INFO);
      if (pDlgArtistInfo)
      {
        pDlgArtistInfo->SetArtist(info.GetArtist(), path);
        if (bShowInfo)
          pDlgArtistInfo->DoModal();
        else
          pDlgArtistInfo->RefreshThumb();  // downloads the thumb if we don't already have one

        CArtist artistInfo = info.GetArtist();
        artistInfo.idArtist = artist.idArtist;
        /*
         if (pDlgAlbumInfo->HasUpdatedThumb())
         UpdateThumb(artistInfo, path);
         */
        // just update for now
        Update(m_vecItems->GetPath());
        if (pDlgArtistInfo->NeedRefresh())
        {
          m_musicdatabase.DeleteArtistInfo(artistInfo.idArtist);
          ShowArtistInfo(artist, path, true, bShowInfo);
          return;
        }
      }
    }
    else
    {
      // failed 2 download album info
      CGUIDialogOK::ShowAndGetInput(21889, 0, 20199, 0);
    }
  }

  if (m_dlgProgress && bShowInfo) m_dlgProgress->Close();
}

void CGUIWindowJukeboxBase::ShowAlbumInfo(const CAlbum& album, const CStdString& path, bool bRefresh, bool bShowInfo) {
  bool saveDb = album.idAlbum != -1;
  if (!g_settings.GetCurrentProfile().canWriteDatabases() && !g_passwordManager.bMasterUser) saveDb = false;

  // check cache
  CAlbum albumInfo;
  if (!bRefresh && m_musicdatabase.GetAlbumInfo(album.idAlbum, albumInfo, &albumInfo.songs))
  {
    if (!bShowInfo) return;

    CGUIDialogMusicInfo *pDlgAlbumInfo = (CGUIDialogMusicInfo*) g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_INFO);
    if (pDlgAlbumInfo)
    {
      pDlgAlbumInfo->SetAlbum(albumInfo, path);
      if (bShowInfo)
        pDlgAlbumInfo->DoModal();
      else
        pDlgAlbumInfo->RefreshThumb();  // downloads the thumb if we don't already have one

      if (!pDlgAlbumInfo->NeedRefresh())
      {
        if (pDlgAlbumInfo->HasUpdatedThumb()) UpdateThumb(albumInfo, path);
        return;
      }
      bRefresh = true;
      m_musicdatabase.DeleteAlbumInfo(albumInfo.idAlbum);
    }
  }

  // If we are scanning for music info in the background,
  // other writing access to the database is prohibited.
  CGUIDialogMusicScan* dlgMusicScan = (CGUIDialogMusicScan*) g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
  if (dlgMusicScan->IsDialogRunning())
  {
    CGUIDialogOK::ShowAndGetInput(189, 14057, 0, 0);
    return;
  }

  CMusicAlbumInfo info;
  if (FindAlbumInfo(album.strAlbum, album.strArtist, info, bShowInfo ? (bRefresh ? SELECTION_FORCED : SELECTION_ALLOWED) : SELECTION_AUTO))
  {
    // download the album info
    if (info.Loaded())
    {
      // set album title from musicinfotag, not the one we got from allmusic.com
      info.SetTitle(album.strAlbum);

      if (saveDb)
      {
        // save to database
        m_musicdatabase.SetAlbumInfo(album.idAlbum, info.GetAlbum(), info.GetSongs());
      }
      if (m_dlgProgress && bShowInfo) m_dlgProgress->Close();

      UpdateThumb(album, path);

      // ok, show album info
      CGUIDialogMusicInfo *pDlgAlbumInfo = (CGUIDialogMusicInfo*) g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_INFO);
      if (pDlgAlbumInfo)
      {
        pDlgAlbumInfo->SetAlbum(info.GetAlbum(), path);
        if (bShowInfo)
          pDlgAlbumInfo->DoModal();
        else
          pDlgAlbumInfo->RefreshThumb();  // downloads the thumb if we don't already have one

        CAlbum albumInfo = info.GetAlbum();
        albumInfo.idAlbum = album.idAlbum;
        if (pDlgAlbumInfo->HasUpdatedThumb()) UpdateThumb(albumInfo, path);

        if (pDlgAlbumInfo->NeedRefresh())
        {
          m_musicdatabase.DeleteAlbumInfo(albumInfo.idAlbum);
          ShowAlbumInfo(album, path, true, bShowInfo);
          return;
        }
      }
    }
    else
    {
      // failed 2 download album info
      CGUIDialogOK::ShowAndGetInput(185, 0, 500, 0);
    }
  }

  if (m_dlgProgress && bShowInfo) m_dlgProgress->Close();
}

void CGUIWindowJukeboxBase::ShowSongInfo(CFileItemPtr pItem) {
  CGUIDialogSongInfo *dialog = (CGUIDialogSongInfo *) g_windowManager.GetWindow(WINDOW_DIALOG_SONG_INFO);
  if (dialog)
  {
    if (!pItem->IsMusicDb()) pItem->LoadMusicTag();
    if (!pItem->HasMusicInfoTag()) return;
    dialog->SetSong(pItem.get());
    dialog->DoModal(GetID());
    if (dialog->NeedsUpdate())
    { // update our file list
      m_vecItems->RemoveDiscCache(GetID());
      Update(m_vecItems->GetPath());
    }
  }
}

/*
 /// \brief Can be overwritten to implement an own tag filling function.
 /// \param items File items to fill
 void CGUIWindowJukeboxBase::OnRetrieveMusicInfo(CFileItemList& items)
 {
 }
 */

/// \brief Retrieve tag information for \e m_vecItems
void CGUIWindowJukeboxBase::RetrieveMusicInfo() {
  unsigned int startTick = XbmcThreads::SystemClockMillis();

  OnRetrieveMusicInfo(*m_vecItems);

  CLog::Log(LOGDEBUG, "RetrieveMusicInfo() took %u msec", XbmcThreads::SystemClockMillis() - startTick);
}

/// \brief Add selected file item to playlist and start playing
/// \param pItem The file item to add
void CGUIWindowJukeboxBase::OnQueueItem(CFileItemPtr pItem) {

#ifdef	IS_PROFESSIONAL // Laureon: Added: Jukebox Credits System
  if (g_jukeboxManager.GetModeManager().CanQueue())
  {
#endif
    // don't re-queue items from playlist window
    if (GetID() == WINDOW_MUSIC_PLAYLIST) return;

    int iOldSize = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).size();

    // add item 2 playlist (make a copy as we alter the queuing state)
    CFileItemPtr item(new CFileItem(*pItem));
    // Laureon: QUICK FIX:
//    CStdString strPath =  item->GetPath();
//
//    CURL strURL(strPath);
//   strPath = strURL.GetWithoutFilename();
//    CStdString strFileName = strURL.GetFileNameWithoutPath();
//    CStdString strFinalPath;
//
//    URIUtils::CreateArchivePath(strFinalPath, );
//    item->SetPath(strFinalPath);

    if (!item->Exists()) return;

    if (item->IsRAR() || item->IsZIP() || item->IsPFC()) // Laureon: Possible Bug: This is pfs can bug when you add some file from a PFS container...
    return;

    //  Allow queuing of unqueueable items
    //  when we try to queue them directly
    if (!item->CanQueue()) item->SetCanQueue(true);

    CLog::Log(LOGDEBUG, "Adding file %s%s to music playlist", item->GetPath().c_str(), item->m_bIsFolder ? " (folder) " : "");
    CFileItemList queuedItems;
    AddItemToPlayList(item, queuedItems);

    // select next item
    // m_viewControl.SetSelectedItem(iItem + 1); // Laureon: TODO: Reimplement this. we have to check the m_viewControl... right now its hardcoded must do it attached to xml's amounts of controls..
    CGUIMessage msg(GUI_MSG_MOVE_OFFSET, GetID(), CONTROL_JUKEBOXLIST, 1);
    OnMessage(msg);

    // if party mode, add items but DONT start playing
    if (g_partyModeManager.IsEnabled())
    {
      g_partyModeManager.AddUserSongs(queuedItems, false);
      return;
    }

    g_playlistPlayer.Add(PLAYLIST_MUSIC, queuedItems);
    if (g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).size() && !g_application.IsPlaying())
    {
      if (m_guiState.get()) m_guiState->SetPlaylistDirectory("playlistmusic://");

      g_playlistPlayer.Reset();
      g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
      g_playlistPlayer.Play(iOldSize); // start playing at the first new item
    }
#ifdef IS_PROFESSIONAL // Laureon: Added: Jukebox Credits System
    g_jukeboxManager.GetModeManager().RegisterQueue();
//    SavePlayList("special://masterprofile/playlist.m3u");
  }
  else
  {
    g_jukeboxManager.GetModeManager().ShowMessage();
  }
#endif

}

/// \brief Add unique file and folders and its subfolders to playlist
/// \param pItem The file item to add
void CGUIWindowJukeboxBase::AddItemToPlayList(const CFileItemPtr &pItem, CFileItemList &queuedItems) {
  if (!pItem->CanQueue() || pItem->IsRAR() || pItem->IsZIP() || pItem->IsPFC() || pItem->IsParentFolder()) // no zip/rar enques thank you!
  return; // Laureon: Added: Above: Filesystem

  // fast lookup is needed here
  queuedItems.SetFastLookup(true);

  if (pItem->IsMusicDb() && pItem->m_bIsFolder && !pItem->IsParentFolder())
  { // we have a music database folder, just grab the "all" item underneath it
    CMusicDatabaseDirectory dir;
    if (!dir.ContainsSongs(pItem->GetPath()))
    { // grab the ALL item in this category
      // Genres will still require 2 lookups, and queuing the entire Genre folder
      // will require 3 lookups (genre, artist, album)
      CFileItemPtr item(new CFileItem(pItem->GetPath() + "-1/", true));
      item->SetCanQueue(true); // workaround for CanQueue() check above
      AddItemToPlayList(item, queuedItems);
      return;
    }
  }
  if (pItem->m_bIsFolder || (g_windowManager.GetActiveWindow() == WINDOW_MUSIC_NAV && pItem->IsPlayList()))
  {
    // Check if we add a locked share
    if (pItem->m_bIsShareOrDrive)
    {
      CFileItem item = *pItem;
      if (!g_passwordManager.IsItemUnlocked(&item, "music")) return;
    }

    // recursive
    CFileItemList items;
    GetDirectory(pItem->GetPath(), items);
    //OnRetrieveMusicInfo(items);
    FormatAndSort(items);
    SetupFanart(items);
    for (int i = 0; i < items.Size(); ++i)
      AddItemToPlayList(items[i], queuedItems);
  }
  else
  {
    if (pItem->IsPlayList())
    {
      auto_ptr<CPlayList> pPlayList(CPlayListFactory::Create(*pItem));
      if (pPlayList.get())
      {
        // load it
        if (!pPlayList->Load(pItem->GetPath()))
        {
          CGUIDialogOK::ShowAndGetInput(6, 0, 477, 0);
          return; //hmmm unable to load playlist?
        }

        CPlayList playlist = *pPlayList;
        for (int i = 0; i < (int) playlist.size(); ++i)
        {
          AddItemToPlayList(playlist[i], queuedItems);
        }
        return;
      }
    }
    else if (pItem->IsInternetStream())
    { // just queue the internet stream, it will be expanded on play
      queuedItems.Add(pItem);
    }
    else if (pItem->IsPlugin() && pItem->GetProperty("isplayable") == "true")
    {
      // python files can be played
      queuedItems.Add(pItem);
    }
    else if (!pItem->IsNFO() && (pItem->IsAudio() || pItem->IsVideo()))
    {
      CFileItemPtr itemCheck = queuedItems.Get(pItem->GetPath());
      if (!itemCheck || itemCheck->m_lStartOffset != pItem->m_lStartOffset)
      { // add item
        CFileItemPtr item(new CFileItem(*pItem));
        m_musicdatabase.SetPropertiesForFileItem(*item);
        queuedItems.Add(item);
      }
    }
  }
}

void CGUIWindowJukeboxBase::UpdateButtons() {
  // Update window selection control

  // Remove labels from the window selection
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_BTNTYPE);
  g_windowManager.SendMessage(msg);

  // Add labels to the window selection
  CGUIMessage msg2(GUI_MSG_LABEL_ADD, GetID(), CONTROL_BTNTYPE);
  msg2.SetLabel(g_localizeStrings.Get(744)); // Files
  g_windowManager.SendMessage(msg2);

  msg2.SetLabel(g_localizeStrings.Get(15100)); // Library
  g_windowManager.SendMessage(msg2);

  // Select the current window as default item
  CONTROL_SELECT_ITEM(CONTROL_BTNTYPE, g_settings.m_iMyMusicStartWindow - WINDOW_MUSIC_FILES);

  CGUIMediaWindow::UpdateButtons();
}

bool CGUIWindowJukeboxBase::FindAlbumInfo(const CStdString& strAlbum, const CStdString& strArtist, CMusicAlbumInfo& album, ALLOW_SELECTION allowSelection) {
  // show dialog box indicating we're searching the album
  if (m_dlgProgress && allowSelection != SELECTION_AUTO)
  {
    m_dlgProgress->SetHeading(185);
    m_dlgProgress->SetLine(0, strAlbum);
    m_dlgProgress->SetLine(1, strArtist);
    m_dlgProgress->SetLine(2, "");
    m_dlgProgress->StartModal();
  }

  CMusicInfoScanner scanner;
  CStdString strPath;
  CStdString strTempAlbum(strAlbum);
  CStdString strTempArtist(strArtist);
  long idAlbum = m_musicdatabase.GetAlbumByName(strAlbum, strArtist);
  strPath.Format("musicdb://3/%d/", idAlbum);

  bool bCanceled(false);
  bool needsRefresh(true);
  do
  {
    if (!scanner.DownloadAlbumInfo(strPath, strTempArtist, strTempAlbum, bCanceled, album, m_dlgProgress))
    {
      if (bCanceled) return false;
      if (m_dlgProgress && allowSelection != SELECTION_AUTO)
      {
        if (!CGUIDialogKeyboard::ShowAndGetInput(strTempAlbum, g_localizeStrings.Get(16011), false)) return false;

        if (!CGUIDialogKeyboard::ShowAndGetInput(strTempArtist, g_localizeStrings.Get(16025), false)) return false;
      }
      else
        needsRefresh = false;
    }
    else
      needsRefresh = false;
  } while (needsRefresh || bCanceled);

  // Read the album information from the database if we are dealing with a DB album.
  if (idAlbum != -1) m_musicdatabase.GetAlbumInfo(idAlbum, album.GetAlbum(), &album.GetAlbum().songs);

  album.SetLoaded(true);
  return true;
}

bool CGUIWindowJukeboxBase::FindArtistInfo(const CStdString& strArtist, CMusicArtistInfo& artist, ALLOW_SELECTION allowSelection) {
  // show dialog box indicating we're searching the album
  if (m_dlgProgress && allowSelection != SELECTION_AUTO)
  {
    m_dlgProgress->SetHeading(21889);
    m_dlgProgress->SetLine(0, strArtist);
    m_dlgProgress->SetLine(1, "");
    m_dlgProgress->SetLine(2, "");
    m_dlgProgress->StartModal();
  }

  CMusicInfoScanner scanner;
  CStdString strPath;
  CStdString strTempArtist(strArtist);
  long idArtist = m_musicdatabase.GetArtistByName(strArtist);
  strPath.Format("musicdb://2/%u/", idArtist);

  bool bCanceled(false);
  bool needsRefresh(true);
  do
  {
    if (!scanner.DownloadArtistInfo(strPath, strTempArtist, bCanceled, m_dlgProgress))
    {
      if (bCanceled) return false;
      if (m_dlgProgress && allowSelection != SELECTION_AUTO)
      {
        if (!CGUIDialogKeyboard::ShowAndGetInput(strTempArtist, g_localizeStrings.Get(16025), false)) return false;
      }
      else
        needsRefresh = false;
    }
    else
      needsRefresh = false;
  } while (needsRefresh || bCanceled);

  m_musicdatabase.GetArtistInfo(idArtist, artist.GetArtist());
  artist.SetLoaded();
  return true;
}

void CGUIWindowJukeboxBase::GetContextButtons(CFileItemPtr pItem, CContextButtons &buttons) {
//  if (pItem && !pItem->GetProperty("pluginreplacecontextitems").asBoolean())
//  {
//    if (pItem && !pItem->IsParentFolder())
//    {
//      if (pItem->GetExtraInfo().Equals("lastfmloved"))
//      {
//        buttons.Add(CONTEXT_BUTTON_LASTFM_UNLOVE_ITEM, 15295); //unlove
//      }
//      else if (pItem->GetExtraInfo().Equals("lastfmbanned"))
//      {
//        buttons.Add(CONTEXT_BUTTON_LASTFM_UNBAN_ITEM, 15296); //unban
//      }
//      else if (pItem->CanQueue() && !pItem->IsAddonsPath() && !pItem->IsScript())
//      {
//        buttons.Add(CONTEXT_BUTTON_QUEUE_ITEM, 13347); //queue
//
//        // allow a folder to be ad-hoc queued and played by the default player
//        if (pItem->m_bIsFolder || (pItem->IsPlayList() && !g_advancedSettings.m_playlistAsFolders))
//        {
//          buttons.Add(CONTEXT_BUTTON_PLAY_ITEM, 208); // Play
//        }
//        else
//        { // check what players we have, if we have multiple display play with option
//          VECPLAYERCORES vecCores;
//          CPlayerCoreFactory::GetPlayers(*pItem, vecCores);
//          if (vecCores.size() >= 1)
//            buttons.Add(CONTEXT_BUTTON_PLAY_WITH, 15213); // Play With...
//        }
//        if (pItem->IsSmartPlayList())
//        {
//            buttons.Add(CONTEXT_BUTTON_PLAY_PARTYMODE, 15216); // Play in Partymode
//        }
//
//        if (pItem->IsSmartPlayList() || m_vecItems->IsSmartPlayList())
//          buttons.Add(CONTEXT_BUTTON_EDIT_SMART_PLAYLIST, 586);
//        else if (pItem->IsPlayList() || m_vecItems->IsPlayList())
//          buttons.Add(CONTEXT_BUTTON_EDIT, 586);
//      }
//    }
//  }

#ifdef IS_JUKEBOX
  buttons.Add(CONTEXT_BUTTON_SET_VISIBLE_ALBUM, 71001);
  buttons.Add(CONTEXT_BUTTON_SET_VISIBLE_TRACK, 71002);
#endif

  CGUIMediaWindow::GetContextButtons(pItem, buttons);
}

void CGUIWindowJukeboxBase::GetNonContextButtons(CContextButtons &buttons) {
//  if (!m_vecItems->IsVirtualDirectoryRoot())
//    buttons.Add(CONTEXT_BUTTON_GOTO_ROOT, 20128);
//  if (g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).size() > 0)
//    buttons.Add(CONTEXT_BUTTON_NOW_PLAYING, 13350);
  buttons.Add(CONTEXT_BUTTON_SETTINGS, 5);
}

bool CGUIWindowJukeboxBase::OnContextButton(CFileItemPtr pItem, CONTEXT_BUTTON button) {
  switch (button)
  {
#ifdef IS_JUKEBOX
    case CONTEXT_BUTTON_SET_VISIBLE_ALBUM:
      m_musicdatabase.SetAlbumVisible(pItem->GetMusicInfoTag()->GetDatabaseId(), false);
//    OnQueueItem(pItem);
      return true;
    case CONTEXT_BUTTON_SET_VISIBLE_TRACK:
      OnQueueItem(pItem);
      return true;
#endif

    case CONTEXT_BUTTON_QUEUE_ITEM:
      OnQueueItem(pItem);
      return true;

    case CONTEXT_BUTTON_INFO:
      OnInfo(pItem);
      return true;

//  case CONTEXT_BUTTON_SONG_INFO:
//    {
//      ShowSongInfo(pItem);
//      return true;
//    }

    case CONTEXT_BUTTON_EDIT: {
      CStdString playlist = pItem->IsPlayList() ? pItem->GetPath() : m_vecItems->GetPath(); // save path as activatewindow will destroy our items
      g_windowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST_EDITOR, playlist);
      // need to update
      m_vecItems->RemoveDiscCache(GetID());
      return true;
    }

    case CONTEXT_BUTTON_EDIT_SMART_PLAYLIST: {
      CStdString playlist = pItem->IsSmartPlayList() ? pItem->GetPath() : m_vecItems->GetPath(); // save path as activatewindow will destroy our items
      if (CGUIDialogSmartPlaylistEditor::EditPlaylist(playlist, "music"))
      { // need to update
        m_vecItems->RemoveDiscCache(GetID());
        Update(m_vecItems->GetPath());
      }
      return true;
    }

//  case CONTEXT_BUTTON_PLAY_ITEM:
//    PlayItem(pItem);
//    return true;

//  case CONTEXT_BUTTON_PLAY_WITH:
//    {
//      VECPLAYERCORES vecCores;  // base class?
//      CPlayerCoreFactory::GetPlayers(*pItem, vecCores);
//      g_application.m_eForcedNextPlayer = CPlayerCoreFactory::SelectPlayerDialog(vecCores);
//      if( g_application.m_eForcedNextPlayer != EPC_NONE )
//        OnClick(pItem);
//      return true;
//    }

//  case CONTEXT_BUTTON_PLAY_PARTYMODE:
//    g_partyModeManager.Enable(PARTYMODECONTEXT_MUSIC, pItem->GetPath());
//    return true;

    case CONTEXT_BUTTON_STOP_SCANNING: {
      CGUIDialogMusicScan *scanner = (CGUIDialogMusicScan *) g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
      if (scanner) scanner->StopScanning();
      return true;
    }

//  case CONTEXT_BUTTON_NOW_PLAYING:
//    g_windowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);
//    return true;

    case CONTEXT_BUTTON_GOTO_ROOT:
      Update("");
      return true;

    case CONTEXT_BUTTON_SETTINGS:
      g_windowManager.ActivateWindow(WINDOW_SETTINGS_MYMUSIC);
      return true;

    case CONTEXT_BUTTON_LASTFM_UNBAN_ITEM:
      if (CLastFmManager::GetInstance()->Unban(*pItem->GetMusicInfoTag()))
      {
        g_directoryCache.ClearDirectory(m_vecItems->GetPath());
        m_vecItems->RemoveDiscCache(GetID());
        Update(m_vecItems->GetPath());
      }
      return true;
    case CONTEXT_BUTTON_LASTFM_UNLOVE_ITEM:
      if (CLastFmManager::GetInstance()->Unlove(*pItem->GetMusicInfoTag()))
      {
        g_directoryCache.ClearDirectory(m_vecItems->GetPath());
        m_vecItems->RemoveDiscCache(GetID());
        Update(m_vecItems->GetPath());
      }
      return true;
    default:
    break;
  }

  return CGUIMediaWindow::OnContextButton(pItem, button);
}

void CGUIWindowJukeboxBase::OnRipCD() {
  if (g_mediaManager.IsAudio())
  {
    if (!g_application.CurrentFileItem().IsCDDA())
    {
#ifdef HAS_CDDA_RIPPER
      CCDDARipper ripper;
      ripper.RipCD();
#endif
    }
    else
      CGUIDialogOK::ShowAndGetInput(257, 20099, 0, 0);
  }
}

void CGUIWindowJukeboxBase::OnRipTrack(CFileItemPtr pItem) {
  if (g_mediaManager.IsAudio())
  {
    if (!g_application.CurrentFileItem().IsCDDA())
    {
#ifdef HAS_CDDA_RIPPER
      CCDDARipper ripper;
      ripper.RipTrack(pItem.get());
#endif
    }
    else
      CGUIDialogOK::ShowAndGetInput(257, 20099, 0, 0);
  }
}

void CGUIWindowJukeboxBase::PlayItem(CFileItemPtr pItem) {
  // restrictions should be placed in the appropiate window code
  // only call the base code if the item passes since this clears
  // the current playlist

  // special case for DAAP playlist folders
  bool bIsDAAPplaylist = false;
#ifdef HAS_FILESYSTEM_DAAP
  if (pItem->IsDAAP() && pItem->m_bIsFolder)
  {
    CDAAPDirectory dirDAAP;
    if (dirDAAP.GetCurrLevel(pItem->GetPath()) == 0) bIsDAAPplaylist = true;
  }
#endif
  // if its a folder, build a playlist // Laureon: TODO: CHECK: Do wee need this?? This is for building playlist from folders... Or one could add playlist building for plugins here..
  if ((pItem->m_bIsFolder && !pItem->IsPlugin()) || (g_windowManager.GetActiveWindow() == WINDOW_MUSIC_NAV && pItem->IsPlayList()))
  {
    // make a copy so that we can alter the queue state
    CFileItemPtr item(new CFileItem(*pItem));

    //  Allow queuing of unqueueable items
    //  when we try to queue them directly
    if (!item->CanQueue()) item->SetCanQueue(true);

    // skip ".."
    if (item->IsParentFolder()) return;

    CFileItemList queuedItems;
    AddItemToPlayList(item, queuedItems);
    if (g_partyModeManager.IsEnabled())
    {
      g_partyModeManager.AddUserSongs(queuedItems, true);
      return;
    }

    /*
     CStdString strPlayListDirectory = m_vecItems->m_strPath;
     CUtil::RemoveSlashAtEnd(strPlayListDirectory);
     */

    g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
    g_playlistPlayer.Reset();
    g_playlistPlayer.Add(PLAYLIST_MUSIC, queuedItems);
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);

    // activate the playlist window if its not activated yet
    if (bIsDAAPplaylist && GetID() == g_windowManager.GetActiveWindow()) g_windowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);

    // play!
    g_playlistPlayer.Play();
  }
  else if (pItem->IsPlayList())
  {
    // load the playlist the old way
    LoadPlayList(pItem->GetPath());
  }
  else
  {
    // just a single item, play it
    // TODO: Add music-specific code for single playback of an item here (See OnClick in JukeboxWindow, and OnPlayMedia below)
    OnClick(pItem);
  }
}

void CGUIWindowJukeboxBase::LoadPlayList(const CStdString& strPlayList) {
  // if partymode is active, we disable it
//  if (g_partyModeManager.IsEnabled())
//    g_partyModeManager.Disable();

  // load a playlist like .m3u, .pls
  // first get correct factory to load playlist
  auto_ptr<CPlayList> pPlayList(CPlayListFactory::Create(strPlayList));
  if (pPlayList.get())
  {
    // load it
    if (!pPlayList->Load(strPlayList))
    {
      CGUIDialogOK::ShowAndGetInput(6, 0, 477, 0);
      return; //hmmm unable to load playlist?
    }
  }

  int iSize = pPlayList->size();
  g_application.ProcessAndStartPlaylist(strPlayList, *pPlayList, PLAYLIST_MUSIC);
//  if (g_application.ProcessAndStartPlaylist(strPlayList, *pPlayList, PLAYLIST_MUSIC))
//  {
//    if (m_guiState.get())
//      m_guiState->SetPlaylistDirectory("playlistmusic://");
//    // activate the playlist window if its not activated yet
//    if (GetID() == g_windowManager.GetActiveWindow() && iSize > 1)
//    {
//      g_windowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);
//    }
//  }
}

void CGUIWindowJukeboxBase::SavePlayList(const CStdString& strPlayList) {
  CPlayListM3U playlist((CPlayListM3U&) g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC));
  playlist.Save(strPlayList);
}

bool CGUIWindowJukeboxBase::OnPlayMedia(CFileItemPtr pItem) {
  // party mode
  if (g_partyModeManager.IsEnabled() && !pItem->IsLastFM())
  {
    CPlayList playlistTemp;
    playlistTemp.Add(pItem);
    g_partyModeManager.AddUserSongs(playlistTemp, true);
    return true;
  }
  else if (!pItem->IsPlayList() && !pItem->IsInternetStream())
  { // single music file - if we get here then we have autoplaynextitem turned off or queuebydefault
    // turned on, but we still want to use the playlist player in order to handle more queued items
    // following etc.
    // Karaoke items also can be added in runtime (while the song is played), so it should be queued too.
    if ((g_guiSettings.GetBool("musicplayer.queuebydefault") && g_windowManager.GetActiveWindow() != WINDOW_MUSIC_PLAYLIST_EDITOR) || pItem->IsKaraoke())
    {
      // TODO: Should the playlist be cleared if nothing is already playing?
      OnQueueItem(pItem);
      return true;
    }
    g_playlistPlayer.Reset();
    g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
    g_playlistPlayer.Add(PLAYLIST_MUSIC, pItem);
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
    g_playlistPlayer.Play();
    return true;
  }
  return CGUIMediaWindow::OnPlayMedia(pItem);
}

void CGUIWindowJukeboxBase::UpdateThumb(const CAlbum &album, const CStdString &path) {
  // check user permissions
  bool saveDb = album.idAlbum != -1;
  bool saveDirThumb = true;
  if (!g_settings.GetCurrentProfile().canWriteDatabases() && !g_passwordManager.bMasterUser)
  {
    saveDb = false;
    saveDirThumb = false;
  }

  CStdString albumThumb(CThumbnailCache::GetAlbumThumb(album));

  // Update the thumb in the music database (songs + albums)
  CStdString albumPath(path);
  if (saveDb && CFile::Exists(albumThumb)) m_musicdatabase.SaveAlbumThumb(album.idAlbum, albumThumb);

  // Update currently playing song if it's from the same album.  This is necessary as when the album
  // first gets it's cover, the info manager's item doesn't have the updated information (so will be
  // sending a blank thumb to the skin.)
  if (g_application.IsPlayingAudio())
  {
    const CMusicInfoTag* tag = g_infoManager.GetCurrentSongTag();
    if (tag)
    {
      // really, this may not be enough as it is to reliably update this item.  eg think of various artists albums
      // that aren't tagged as such (and aren't yet scanned).  But we probably can't do anything better than this
      // in that case
      if (album.strAlbum == tag->GetAlbum() && (album.strArtist == tag->GetAlbumArtist() || album.strArtist == tag->GetArtist()))
      {
        g_infoManager.SetCurrentAlbumThumb(albumThumb);
      }
    }
  }

  // Save this thumb as the directory thumb if it's the only album in the folder (files view nicety)
  // We do this by grabbing all the songs in the folder, and checking to see whether they come
  // from the same album.
  if (saveDirThumb && CFile::Exists(albumThumb) && !albumPath.IsEmpty() && !URIUtils::IsCDDA(albumPath))
  {
    CFileItemList items;
    GetDirectory(albumPath, items);
    OnRetrieveMusicInfo(items);
    VECSONGS songs;
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr item = items[i];
      if (item->HasMusicInfoTag() && item->GetMusicInfoTag()->Loaded())
      {
        CSong song(*item->GetMusicInfoTag());
        songs.push_back(song);
      }
    }
    CMusicInfoScanner::CheckForVariousArtists(songs);
    CStdString album, artist;
    if (CMusicInfoScanner::HasSingleAlbum(songs, album, artist))
    { // can cache as the folder thumb
      CStdString folderThumb(CThumbnailCache::GetMusicThumb(albumPath));
      CFile::Cache(albumThumb, folderThumb);
    }
  }

  // update the file listing - we have to update the whole lot, as it's likely that
  // more than just our thumbnaias changed
  // TODO: Ideally this would only be done when needed - at the moment we appear to be
  //       doing this for every lookup, possibly twice (see ShowAlbumInfo)
  m_vecItems->RemoveDiscCache(GetID());
  Update(m_vecItems->GetPath());

  //  Do we have to autoswitch to the thumb control?
  m_guiState.reset(CGUIViewState::GetViewState(GetID(), *m_vecItems));
  UpdateButtons();
}

void CGUIWindowJukeboxBase::OnRetrieveMusicInfo(CFileItemList& items) {
  if (items.GetFolderCount() == items.Size() || items.IsMusicDb() || (!g_guiSettings.GetBool("musicfiles.usetags") && !items.IsCDDA()))
  {
    return;
  }
  // Start the music info loader thread
  m_musicInfoLoader.SetProgressCallback(m_dlgProgress);
  m_musicInfoLoader.Load(items);

  bool bShowProgress = !g_windowManager.HasModalDialog();
  bool bProgressVisible = false;

  unsigned int tick = XbmcThreads::SystemClockMillis();

  while (m_musicInfoLoader.IsLoading())
  {
    if (bShowProgress)
    { // Do we have to init a progress dialog?
      unsigned int elapsed = XbmcThreads::SystemClockMillis() - tick;

      if (!bProgressVisible && elapsed > 1500 && m_dlgProgress)
      { // tag loading takes more then 1.5 secs, show a progress dialog
        CURL url(items.GetPath());
        CStdString strStrippedPath = url.GetWithoutUserDetails();
        m_dlgProgress->SetHeading(189);
        m_dlgProgress->SetLine(0, 505);
        m_dlgProgress->SetLine(1, "");
        m_dlgProgress->SetLine(2, strStrippedPath);
        m_dlgProgress->StartModal();
        m_dlgProgress->ShowProgressBar(true);
        bProgressVisible = true;
      }

      if (bProgressVisible && m_dlgProgress && !m_dlgProgress->IsCanceled())
      { // keep GUI alive
        m_dlgProgress->Progress();
      }
    } // if (bShowProgress)
    Sleep(1);
  } // while (m_musicInfoLoader.IsLoading())

  if (bProgressVisible && m_dlgProgress) m_dlgProgress->Close();
}

bool CGUIWindowJukeboxBase::GetDirectory(const CStdString &strDirectory, CFileItemList &items) {
  items.SetThumbnailImage("");
  bool bResult = CGUIMediaWindow::GetDirectory(strDirectory, items);
  if (bResult) items.SetMusicThumb();

  // add in the "New Playlist" item if we're in the playlists folder
  if ((items.GetPath() == "special://musicplaylists/") && !items.Contains("newplaylist://"))
  {
    CFileItemPtr newPlaylist(new CFileItem(g_settings.GetUserDataItem("PartyMode.xsp"), false));
    newPlaylist->SetLabel(g_localizeStrings.Get(16035));
    newPlaylist->SetLabelPreformated(true);
    newPlaylist->m_bIsFolder = true;
    items.Add(newPlaylist);

    newPlaylist.reset(new CFileItem("newplaylist://", false));
    newPlaylist->SetLabel(g_localizeStrings.Get(525));
    newPlaylist->SetLabelPreformated(true);
    newPlaylist->SetSpecialSort(SORT_ON_BOTTOM);
    newPlaylist->SetCanQueue(false);
    items.Add(newPlaylist);

    newPlaylist.reset(new CFileItem("newsmartplaylist://music", false));
    newPlaylist->SetLabel(g_localizeStrings.Get(21437));
    newPlaylist->SetLabelPreformated(true);
    newPlaylist->SetSpecialSort(SORT_ON_BOTTOM);
    newPlaylist->SetCanQueue(false);
    items.Add(newPlaylist);
  }

  return bResult;
}

bool CGUIWindowJukeboxBase::GetMusicVideos(CFileItemPtr pItem, CFileItemList &items) {
  GetDirectory(pItem->GetPath(), items);

  if (pItem->IsAlbum()) // Laureon: Jukebox: Music Video System: Testing if an item is an album first to list its musics
    m_videodatabase.GetMusicVideosByAlbum(pItem->GetMusicInfoTag()->GetAlbum(), items);
}

void CGUIWindowJukeboxBase::OnPrepareFileItems(CFileItemList &items) {
  if (!items.GetPath().Equals("plugin://music/")) items.SetCachedMusicThumbs();
}

void CGUIWindowJukeboxBase::SetupFanart(CFileItemList& items) {
  // set fanart
  map<CStdString, CStdString> artists;
  for (int i = 0; i < items.Size(); i++)
  {
    CFileItemPtr item = items[i];
    CStdString strArtist;
    if (item->HasProperty("fanart_image")) continue;
    if (item->HasMusicInfoTag()) strArtist = item->GetMusicInfoTag()->GetArtist();
    if (item->HasVideoInfoTag()) strArtist = item->GetVideoInfoTag()->m_strArtist;
    if (strArtist.IsEmpty()) continue;
    map<CStdString, CStdString>::iterator artist = artists.find(strArtist);
    if (artist == artists.end())
    {
      CStdString strFanart = item->GetCachedFanart();
      if (XFILE::CFile::Exists(strFanart))
        item->SetProperty("fanart_image", strFanart);
      else
        strFanart = "";
      artists.insert(make_pair(strArtist, strFanart));
    }
    else
      item->SetProperty("fanart_image", artist->second);
  }
}

CStdString CGUIWindowJukeboxBase::GetStartFolder(const CStdString &dir) {
  if (dir.Equals("Plugins") || dir.Equals("Addons"))
    return "addons://sources/audio/";
  else if (dir.Equals("$PLAYLISTS") || dir.Equals("Playlists")) return "special://musicplaylists/";
  return CGUIMediaWindow::GetStartFolder(dir);
}
