

#include "system.h"
#include "GUIWindowScreensaver.h"
#include "addons/AddonManager.h"
#include "Application.h"
#include "GUIPassword.h"
#include "settings/GUISettings.h"
#include "GUIUserMessages.h"
#include "guilib/GUIWindowManager.h"
#include "threads/SingleLock.h"

using namespace ADDON;

CGUIWindowScreensaver::CGUIWindowScreensaver(void)
    : CGUIWindow(WINDOW_SCREENSAVER, "")
{
}

CGUIWindowScreensaver::~CGUIWindowScreensaver(void)
{
}

void CGUIWindowScreensaver::Process(unsigned int currentTime, CDirtyRegionList &regions)
{
  MarkDirtyRegion();
  CGUIWindow::Process(currentTime, regions);
  m_renderRegion.SetRect(0, 0, (float)g_graphicsContext.GetWidth(), (float)g_graphicsContext.GetHeight());
}

void CGUIWindowScreensaver::Render()
{
  CSingleLock lock (m_critSection);

#ifdef HAS_SCREENSAVER
  if (m_addon)
  {
    if (m_bInitialized)
    {
      try
      {
        //some screensavers seem to be depending on xbmc clearing the screen
        //       g_Windowing.Get3DDevice()->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0x00010001, 1.0f, 0L );
        g_graphicsContext.CaptureStateBlock();
        m_addon->Render();
        g_graphicsContext.ApplyStateBlock();
      }
      catch (...)
      {
        CLog::Log(LOGERROR, "SCREENSAVER: - Exception in Render() - %s", m_addon->Name().c_str());
      }
      return ;
    }
    else
    {
      try
      {
        m_addon->Start();
        m_bInitialized = true;
      }
      catch (...)
      {
        CLog::Log(LOGERROR, "SCREENSAVER: - Exception in Start() - %s", m_addon->Name().c_str());
      }
      return ;
    }
  }
#endif
  CGUIWindow::Render();
}

bool CGUIWindowScreensaver::OnAction(const CAction &action)
{
  // We're just a screen saver, nothing to do here
  return false;
}

// called when the mouse is moved/clicked etc. etc.
EVENT_RESULT CGUIWindowScreensaver::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  g_windowManager.PreviousWindow();
  return EVENT_RESULT_HANDLED;
}

bool CGUIWindowScreensaver::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      CSingleLock lock (m_critSection);
#ifdef HAS_SCREENSAVER
      if (m_addon)
      {
        m_addon->Stop();
        g_graphicsContext.ApplyStateBlock();
        m_addon->Destroy();
        m_addon.reset();
      }
#endif
      m_bInitialized = false;

      // remove z-buffer
//      RESOLUTION res = g_graphicsContext.GetVideoResolution();
 //     g_graphicsContext.SetVideoResolution(res, FALSE);

      // enable the overlay
      g_windowManager.ShowOverlay(OVERLAY_STATE_SHOWN);
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);
      CSingleLock lock (m_critSection);

#ifdef HAS_SCREENSAVER
      assert(!m_addon);
      m_bInitialized = false;

      m_addon.reset();
      // Setup new screensaver instance
      AddonPtr addon;
      if (!CAddonMgr::Get().GetAddon(g_guiSettings.GetString("screensaver.mode"), addon, ADDON_SCREENSAVER))
        return false;

      m_addon = boost::dynamic_pointer_cast<CScreenSaver>(addon);

      if (!m_addon)
        return false;

      g_graphicsContext.CaptureStateBlock();
      m_addon->CreateScreenSaver();
#endif
      // setup a z-buffer
//      RESOLUTION res = g_graphicsContext.GetVideoResolution();
//      g_graphicsContext.SetVideoResolution(res, TRUE);

      // disable the overlay
      g_windowManager.ShowOverlay(OVERLAY_STATE_HIDDEN);
      return true;
    }
  case GUI_MSG_CHECK_LOCK:
    if (!g_passwordManager.IsProfileLockUnlocked())
    {
      g_application.m_iScreenSaveLock = -1;
      return false;
    }
    g_application.m_iScreenSaveLock = 1;
    return true;
  }
  return CGUIWindow::OnMessage(message);
}

