
#include "AutorunMediaJob.h"
#include "Application.h"
#include "interfaces/Builtins.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/Key.h"

CAutorunMediaJob::CAutorunMediaJob(const CStdString &label, const CStdString &path)
{
  m_label = label;
  m_path  = path;
}

bool CAutorunMediaJob::DoWork()
{
  CGUIDialogSelect* pDialog= (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);

  // wake up and turn off the screensaver if it's active
  g_application.WakeUpScreenSaverAndDPMS();

  pDialog->Reset();
  if (m_label.size() > 0)
    pDialog->SetHeading(m_label);
  else
    pDialog->SetHeading("New media detected");

  pDialog->Add("Browse videos");
  pDialog->Add("Browse music");
  pDialog->Add("Browse pictures");
  pDialog->Add("Browse files");

  pDialog->DoModal();

  int selection = pDialog->GetSelectedLabel();
  if (selection >= 0)
  {
    CStdString strAction;
    strAction.Format("ActivateWindow(%s, %s)", GetWindowString(selection), m_path.c_str());
    CBuiltins::Execute(strAction);
  }

  return true;
}

const char *CAutorunMediaJob::GetWindowString(int selection)
{
  switch (selection)
  {
    case 0:
      return "VideoFiles";
    case 1:
      return "MusicFiles";
    case 2:
      return "Pictures";
    case 3:
      return "FileManager";
    default:
      return "FileManager";
  }
}
