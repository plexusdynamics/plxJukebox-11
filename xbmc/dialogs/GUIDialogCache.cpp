
 
#include "threads/SystemClock.h"
#include "GUIDialogCache.h"
#include "Application.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogProgress.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"
#include "threads/SingleLock.h"
#include "utils/TimeUtils.h"

CGUIDialogCache::CGUIDialogCache(DWORD dwDelay, const CStdString& strHeader, const CStdString& strMsg) 
{
  m_pDlg = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

  /* if progress dialog is already running, take it over */
  if( m_pDlg->IsDialogRunning() )
    dwDelay = 0;

  m_strHeader = strHeader;
  m_strLinePrev = strMsg;
  bSentCancel = false;

  if(dwDelay == 0)
    OpenDialog();    
  else
    m_endtime.Set((unsigned int)dwDelay);

  Create(true);
}

void CGUIDialogCache::Close(bool bForceClose)
{
  bSentCancel = true;

  // we cannot wait for the app thread to process the close message
  // as this might happen during player startup which leads to a deadlock
  if (m_pDlg->IsDialogRunning())
    g_application.getApplicationMessenger().Close(m_pDlg,bForceClose,false);

  //Set stop, this will kill this object, when thread stops  
  CThread::m_bStop = true;
}

CGUIDialogCache::~CGUIDialogCache()
{
  if(m_pDlg->IsDialogRunning())
    m_pDlg->Close();
}

void CGUIDialogCache::OpenDialog()
{  
  if (m_strHeader.IsEmpty())
    m_pDlg->SetHeading(438);
  else
    m_pDlg->SetHeading(m_strHeader);

  m_pDlg->SetLine(2, m_strLinePrev);
  m_pDlg->StartModal();
  bSentCancel = false;
}

void CGUIDialogCache::SetHeader(const CStdString& strHeader)
{
  m_strHeader = strHeader;
}

void CGUIDialogCache::SetHeader(int nHeader)
{
  SetHeader(g_localizeStrings.Get(nHeader));
}

void CGUIDialogCache::SetMessage(const CStdString& strMessage)
{
  m_pDlg->SetLine(0, m_strLinePrev2);
  m_pDlg->SetLine(1, m_strLinePrev);
  m_pDlg->SetLine(2, strMessage);
  m_strLinePrev2 = m_strLinePrev;
  m_strLinePrev = strMessage; 
}

bool CGUIDialogCache::OnFileCallback(void* pContext, int ipercent, float avgSpeed)
{
  m_pDlg->ShowProgressBar(true);
  m_pDlg->SetPercentage(ipercent); 

  if( IsCanceled() ) 
    return false;
  else
    return true;
}

void CGUIDialogCache::Process()
{
  while( true )
  {
    
    { //Section to make the lock go out of scope before sleep
      
      if( CThread::m_bStop ) break;

      try 
      {
        CSingleLock lock(g_graphicsContext);
        m_pDlg->Progress();
        if( bSentCancel )
        {
          Sleep(10);
          continue;
        }

        if(m_pDlg->IsCanceled())
        {
          bSentCancel = true;
        }
        else if( !m_pDlg->IsDialogRunning() && m_endtime.IsTimePast() 
              && !g_windowManager.IsWindowActive(WINDOW_DIALOG_YES_NO) )
          OpenDialog();
      }
      catch(...)
      {
        CLog::Log(LOGERROR, "Exception in CGUIDialogCache::Process()");
      }
    }

    Sleep(10);
  }
}

void CGUIDialogCache::ShowProgressBar(bool bOnOff) 
{ 
  m_pDlg->ShowProgressBar(bOnOff); 
}
void CGUIDialogCache::SetPercentage(int iPercentage) 
{ 
  m_pDlg->SetPercentage(iPercentage); 
}
bool CGUIDialogCache::IsCanceled() const
{
  if (m_pDlg->IsDialogRunning())
    return m_pDlg->IsCanceled();
  else
    return false;
}
