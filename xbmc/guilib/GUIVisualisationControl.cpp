

#include "GUIVisualisationControl.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "Application.h"
#include "addons/AddonManager.h"
#include "addons/Visualisation.h"
#include "utils/log.h"

using namespace std;
using namespace ADDON;

#define LABEL_ROW1 10
#define LABEL_ROW2 11
#define LABEL_ROW3 12

CGUIVisualisationControl::CGUIVisualisationControl(int parentID, int controlID, float posX, float posY, float width, float height)
    : CGUIRenderingControl(parentID, controlID, posX, posY, width, height), m_bAttemptedLoad(false)
{
  ControlType = GUICONTROL_VISUALISATION;
}

CGUIVisualisationControl::CGUIVisualisationControl(const CGUIVisualisationControl &from)
: CGUIRenderingControl(from)
{
  ControlType = GUICONTROL_VISUALISATION;
}

bool CGUIVisualisationControl::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_GET_VISUALISATION:
    message.SetPointer(m_addon.get());
    return m_addon;
  case GUI_MSG_VISUALISATION_RELOAD:
    FreeResources(true);
    return true;
  case GUI_MSG_PLAYBACK_STARTED:
    if (m_addon)
    {
      m_addon->UpdateTrack();
      return true;
    }
    break;
  }
  return CGUIRenderingControl::OnMessage(message);
}

bool CGUIVisualisationControl::OnAction(const CAction &action)
{
  if (!m_addon)
    return false;

  switch (action.GetID())
  {
  case ACTION_VIS_PRESET_NEXT:
    return m_addon->OnAction(VIS_ACTION_NEXT_PRESET);
  case ACTION_VIS_PRESET_PREV:
    return m_addon->OnAction(VIS_ACTION_PREV_PRESET);
  case ACTION_VIS_PRESET_RANDOM:
    return m_addon->OnAction(VIS_ACTION_RANDOM_PRESET);
  case ACTION_VIS_RATE_PRESET_PLUS:
    return m_addon->OnAction(VIS_ACTION_RATE_PRESET_PLUS);
  case ACTION_VIS_RATE_PRESET_MINUS:
    return m_addon->OnAction(VIS_ACTION_RATE_PRESET_MINUS);
  case ACTION_VIS_PRESET_LOCK:
    return m_addon->OnAction(VIS_ACTION_LOCK_PRESET);
  default:
    return CGUIRenderingControl::OnAction(action);
  }
}

void CGUIVisualisationControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (g_application.IsPlayingAudio())
  {
    if (m_bInvalidated)
      FreeResources(true);

    if (!m_addon && !m_bAttemptedLoad)
    {
      AddonPtr viz;
      if (ADDON::CAddonMgr::Get().GetDefault(ADDON_VIZ, viz))
        LoadAddon(viz);

      m_bAttemptedLoad = true;
    }
  }
  CGUIRenderingControl::Process(currentTime, dirtyregions);
}

void CGUIVisualisationControl::FreeResources(bool immediately)
{
  m_bAttemptedLoad = false;
  // tell our app that we're going
  if (!m_addon)
    return;

  CGUIMessage msg(GUI_MSG_VISUALISATION_UNLOADING, m_controlID, 0);
  g_windowManager.SendMessage(msg);
  CLog::Log(LOGDEBUG, "FreeVisualisation() started");
  CGUIRenderingControl::FreeResources(immediately);
  CLog::Log(LOGDEBUG, "FreeVisualisation() done");
}

