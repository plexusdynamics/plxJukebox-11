

#include "Application.h"
#include "guilib/GUIWindowManager.h"
#include "settings/AdvancedSettings.h"

#include "GUIDialogKaraokeSongSelector.h"
#include "GUIWindowKaraokeLyrics.h"
#include "karaokelyrics.h"
#include "karaokewindowbackground.h"
#include "threads/SingleLock.h"


CGUIWindowKaraokeLyrics::CGUIWindowKaraokeLyrics(void)
  : CGUIWindow(WINDOW_KARAOKELYRICS, "MusicKaraokeLyrics.xml")
{
  m_Lyrics = 0;
  m_Background = new CKaraokeWindowBackground();
}


CGUIWindowKaraokeLyrics::~ CGUIWindowKaraokeLyrics(void )
{
  delete m_Background;
}


bool CGUIWindowKaraokeLyrics::OnAction(const CAction &action)
{
  CSingleLock lock (m_CritSection);

  if ( !m_Lyrics || !g_application.IsPlayingAudio() )
    return false;

  CGUIDialogKaraokeSongSelectorSmall * songSelector = (CGUIDialogKaraokeSongSelectorSmall *)
                                      g_windowManager.GetWindow( WINDOW_DIALOG_KARAOKE_SONGSELECT );

  switch(action.GetID())
  {
    case ACTION_SUBTITLE_DELAY_MIN:
      m_Lyrics->lyricsDelayDecrease();
      return true;

    case ACTION_SUBTITLE_DELAY_PLUS:
      m_Lyrics->lyricsDelayIncrease();
      return true;
  
    default:
      if ( CGUIDialogKaraokeSongSelector::GetKeyNumber( action.GetID() ) != -1 && songSelector && !songSelector->IsActive() )
        songSelector->DoModal( CGUIDialogKaraokeSongSelector::GetKeyNumber( action.GetID() ) );

      break;
  }

  // If our background control could handle the action, let it do it
  if ( m_Background && m_Background->OnAction(action) )
    return true;

  return CGUIWindow::OnAction(action);
}


bool CGUIWindowKaraokeLyrics::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      // Must be called here so we get our window ID and controls
      if ( !CGUIWindow::OnMessage(message) )
        return false;

      m_Background->Init( this );
      return true;
    }
    break;

  case GUI_MSG_WINDOW_DEINIT:
    {
      CSingleLock lock (m_CritSection);

      // Close the song selector dialog if shown
      CGUIDialogKaraokeSongSelectorSmall * songSelector = (CGUIDialogKaraokeSongSelectorSmall *)
                                      g_windowManager.GetWindow( WINDOW_DIALOG_KARAOKE_SONGSELECT );

      if ( songSelector && songSelector->IsActive() )
        songSelector->Close();
    }
    break;
  }

  return CGUIWindow::OnMessage(message);
}


void CGUIWindowKaraokeLyrics::Render()
{
  g_application.ResetScreenSaver();
  CGUIWindow::Render();

  CSingleLock lock (m_CritSection);

  if ( m_Lyrics )
  {
    m_Background->Render();
    m_Lyrics->Render();
  }
}


void CGUIWindowKaraokeLyrics::newSong(CKaraokeLyrics * lyrics)
{
  CSingleLock lock (m_CritSection);
  m_Lyrics = lyrics;

  m_Lyrics->InitGraphics();

  // Set up current background mode
  if ( m_Lyrics->HasVideo() )
  {
    CStdString path;
    __int64 offset;

    // Start the required video
    m_Lyrics->GetVideoParameters( path, offset );
    m_Background->StartVideo( path, offset );
  }
  else if ( m_Lyrics->HasBackground() && g_advancedSettings.m_karaokeAlwaysEmptyOnCdgs )
  {
    m_Background->StartEmpty();
  }
  else
  {
    m_Background->StartDefault();
  }
}


void CGUIWindowKaraokeLyrics::stopSong()
{
  CSingleLock lock (m_CritSection);
  m_Lyrics = 0;

  m_Background->Stop();
}

void CGUIWindowKaraokeLyrics::pauseSong(bool now_paused)
{
  CSingleLock lock (m_CritSection);
  m_Background->Pause( now_paused );
}
