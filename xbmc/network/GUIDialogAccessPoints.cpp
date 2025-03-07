

#include "GUIDialogAccessPoints.h"
#include "dialogs/GUIDialogKeyboard.h"
#ifdef _LINUX
#include "linux/NetworkLinux.h"
#endif
#include "Application.h"
#include "FileItem.h"
#include "guilib/LocalizeStrings.h"

#define CONTROL_ACCESS_POINTS 3

CGUIDialogAccessPoints::CGUIDialogAccessPoints(void)
    : CGUIDialog(WINDOW_DIALOG_ACCESS_POINTS, "DialogAccessPoints.xml")
{
  m_accessPoints = new CFileItemList;
}

CGUIDialogAccessPoints::~CGUIDialogAccessPoints(void)
{
  delete m_accessPoints;
}

bool CGUIDialogAccessPoints::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_SELECT_ITEM)
  {
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_ACCESS_POINTS);
    OnMessage(msg);
    int iItem = msg.GetParam1();

    if (iItem == (int) m_aps.size())
    {
       m_selectedAPEssId = "";
       if (CGUIDialogKeyboard::ShowAndGetInput(m_selectedAPEssId, g_localizeStrings.Get(789), false))
       {
         m_selectedAPEncMode = m_aps[iItem].getEncryptionMode();
         m_wasItemSelected = true;
         Close();
         return true;
       }
    }
    else
    {
       m_selectedAPEssId = m_aps[iItem].getEssId();
       m_selectedAPEncMode = m_aps[iItem].getEncryptionMode();
       m_wasItemSelected = true;
       Close();
       return true;
    }
  }

  return CGUIDialog::OnAction(action);
}

void CGUIDialogAccessPoints::OnInitWindow()
{
  m_wasItemSelected = false;

  CGUIDialog::OnInitWindow();

  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_ACCESS_POINTS);
  OnMessage(msgReset);

  m_accessPoints->Clear();

  CStdString ifaceName(m_interfaceName);
  CNetworkInterface* iface = g_application.getNetwork().GetInterfaceByName(ifaceName);
  m_aps = iface->GetAccessPoints();

  for (int i = 0; i < (int) m_aps.size(); i++)
  {
      CFileItemPtr item(new CFileItem(m_aps[i].getEssId()));

      int q = m_aps[i].getQuality();
      if (q <= 20) item->SetThumbnailImage("ap-signal1.png");
      else if (q <= 40) item->SetThumbnailImage("ap-signal2.png");
      else if (q <= 60) item->SetThumbnailImage("ap-signal3.png");
      else if (q <= 80) item->SetThumbnailImage("ap-signal4.png");
      else if (q <= 100) item->SetThumbnailImage("ap-signal5.png");

      if (m_aps[i].getEncryptionMode() != ENC_NONE)
         item->SetIconImage("ap-lock.png");

      m_accessPoints->Add(item);
  }

  CFileItemPtr item(new CFileItem(g_localizeStrings.Get(1047)));
  m_accessPoints->Add(item);

  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_ACCESS_POINTS, 0, 0, m_accessPoints);
  OnMessage(msg);
}

void CGUIDialogAccessPoints::SetInterfaceName(CStdString interfaceName)
{
  m_interfaceName = interfaceName;
}

CStdString CGUIDialogAccessPoints::GetSelectedAccessPointEssId()
{
  return m_selectedAPEssId;
}

EncMode CGUIDialogAccessPoints::GetSelectedAccessPointEncMode()
{
  return m_selectedAPEncMode;
}

bool CGUIDialogAccessPoints::WasItemSelected()
{
  return m_wasItemSelected;
}
