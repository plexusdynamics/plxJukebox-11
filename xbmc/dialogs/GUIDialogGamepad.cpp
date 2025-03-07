

#include "GUIDialogGamepad.h"
#include "utils/md5.h"
#include "guilib/GUIAudioManager.h"
#include "guilib/GUIWindowManager.h"
#include "GUIDialogOK.h"
#include "utils/StringUtils.h"
#include "guilib/LocalizeStrings.h"

CGUIDialogGamepad::CGUIDialogGamepad(void)
    : CGUIDialogBoxBase(WINDOW_DIALOG_GAMEPAD, "DialogGamepad.xml")
{
  m_bCanceled = false;
  m_strUserInput = "";
  m_strPassword = "";
  m_iRetries = 0;
  m_bUserInputCleanup = true;
}

CGUIDialogGamepad::~CGUIDialogGamepad(void)
{}

bool CGUIDialogGamepad::OnAction(const CAction &action)
{
  if ((action.GetButtonCode() >= KEY_BUTTON_A &&
       action.GetButtonCode() <= KEY_BUTTON_RIGHT_TRIGGER) ||
      (action.GetButtonCode() >= KEY_BUTTON_DPAD_UP &&
       action.GetButtonCode() <= KEY_BUTTON_DPAD_RIGHT))
  {
    switch (action.GetButtonCode())
    {
    case KEY_BUTTON_A : m_strUserInput += "A"; break;
    case KEY_BUTTON_B : m_strUserInput += "B"; break;
    case KEY_BUTTON_X : m_strUserInput += "X"; break;
    case KEY_BUTTON_Y : m_strUserInput += "Y"; break;
    case KEY_BUTTON_BLACK : m_strUserInput += "K"; break;
    case KEY_BUTTON_WHITE : m_strUserInput += "W"; break;
    case KEY_BUTTON_LEFT_TRIGGER : m_strUserInput += "("; break;
    case KEY_BUTTON_RIGHT_TRIGGER : m_strUserInput += ")"; break;
    case KEY_BUTTON_DPAD_UP : m_strUserInput += "U"; break;
    case KEY_BUTTON_DPAD_DOWN : m_strUserInput += "D"; break;
    case KEY_BUTTON_DPAD_LEFT : m_strUserInput += "L"; break;
    case KEY_BUTTON_DPAD_RIGHT : m_strUserInput += "R"; break;
    default : return true; break;
    }

    CStdString strHiddenInput(m_strUserInput);
    for (int i = 0; i < (int)strHiddenInput.size(); i++)
    {
      strHiddenInput[i] = m_cHideInputChar;
    }
    SetLine(2, strHiddenInput);
    return true;
  }
  else if (action.GetButtonCode() == KEY_BUTTON_BACK || action.GetID() == ACTION_PREVIOUS_MENU || action.GetID() == ACTION_NAV_BACK)
  {
    m_bConfirmed = false;
    m_bCanceled = true;
    m_strUserInput = "";
    m_bHideInputChars = true;
    Close();
    return true;
  }
  else if (action.GetButtonCode() == KEY_BUTTON_START)
  {
    m_bConfirmed = false;
    m_bCanceled = false;

    CStdString md5pword2;
    XBMC::XBMC_MD5 md5state;
    md5state.append(m_strUserInput);
    md5state.getDigest(md5pword2);

    if (!m_strPassword.Equals(md5pword2))
    {
      // incorrect password entered
      m_iRetries--;

      // don't clean up if the calling code wants the bad user input
      if (m_bUserInputCleanup)
        m_strUserInput = "";
      else
        m_bUserInputCleanup = true;

      m_bHideInputChars = true;
      Close();
      return true;
    }

    // correct password entered
    m_bConfirmed = true;
    m_iRetries = 0;
    m_strUserInput = "";
    m_bHideInputChars = true;
    Close();
    return true;
  }
  else if (action.GetID() >= REMOTE_0 && action.GetID() <= REMOTE_9)
  {
    return true; // unhandled
  }
  else
  {
    return CGUIDialog::OnAction(action);
  }
}

bool CGUIDialogGamepad::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      m_bConfirmed = false;
      m_bCanceled = false;
      m_cHideInputChar = g_localizeStrings.Get(12322).c_str()[0];
      CGUIDialog::OnMessage(message);
      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      m_bConfirmed = false;
      m_bCanceled = false;
    }
    break;
  }
  return CGUIDialogBoxBase::OnMessage(message);
}

// \brief Show gamepad keypad and replace aTextString with user input.
// \param aTextString String to preload into the keyboard accumulator. Overwritten with user input if return=true.
// \param dlgHeading String shown on dialog title. Converts to localized string if contains a positive integer.
// \param bHideUserInput Masks user input as asterisks if set as true.  Currently not yet implemented.
// \return true if successful display and user input. false if unsucessful display, no user input, or canceled editing.
bool CGUIDialogGamepad::ShowAndGetInput(CStdString& aTextString, const CStdString &dlgHeading, bool bHideUserInput)
{
  // Prompt user for input
  CStdString strUserInput = "";
  if (ShowAndVerifyInput(strUserInput, dlgHeading, aTextString, "", "", true, bHideUserInput))
  {
    // user entry was blank
    return false;
  }

  if (strUserInput.IsEmpty())
    // user canceled out
    return false;


  // We should have a string to return
  aTextString = strUserInput;
  return true;
}

// \brief Show gamepad keypad twice to get and confirm a user-entered password string.
// \param strNewPassword String to preload into the keyboard accumulator. Overwritten with user input if return=true.
// \return true if successful display and user input entry/re-entry. false if unsucessful display, no user input, or canceled editing.
bool CGUIDialogGamepad::ShowAndVerifyNewPassword(CStdString& strNewPassword)
{
  // Prompt user for password input
  CStdString strUserInput = "";
  if (ShowAndVerifyInput(strUserInput, "12340", "12330", "12331", "", true, true))
  {
    // TODO: Show error to user saying the password entry was blank
    CGUIDialogOK::ShowAndGetInput(12357, 12358, 0, 0); // Password is empty/blank
    return false;
  }

  if (strUserInput.IsEmpty())
    // user canceled out
    return false;

  // Prompt again for password input, this time sending previous input as the password to verify
  if (!ShowAndVerifyInput(strUserInput, "12341", "12330", "12331", "", false, true))
  {
    // TODO: Show error to user saying the password re-entry failed
    CGUIDialogOK::ShowAndGetInput(12357, 12344, 0, 0); // Password do not match
    return false;
  }

  // password entry and re-entry succeeded
  strNewPassword = strUserInput;
  return true;
}

// \brief Show gamepad keypad and verify user input against strPassword.
// \param strPassword Value to compare against user input.
// \param dlgHeading String shown on dialog title. Converts to localized string if contains a positive integer.
// \param iRetries If greater than 0, shows "Incorrect password, %d retries left" on dialog line 2, else line 2 is blank.
// \return 0 if successful display and user input. 1 if unsucessful input. -1 if no user input or canceled editing.
int CGUIDialogGamepad::ShowAndVerifyPassword(CStdString& strPassword, const CStdString& dlgHeading, int iRetries)
{
  CStdString strLine2 = "";
  if (0 < iRetries)
  {
    // Show a string telling user they have iRetries retries left
    strLine2.Format("%s %i %s", g_localizeStrings.Get(12342).c_str(), iRetries, g_localizeStrings.Get(12343).c_str());
  }

  // make a copy of strPassword to prevent from overwriting it later
  CStdString strPassTemp = strPassword;
  if (ShowAndVerifyInput(strPassTemp, dlgHeading, g_localizeStrings.Get(12330), g_localizeStrings.Get(12331), strLine2, true, true))
  {
    // user entered correct password
    return 0;
  }

  if (strPassTemp.IsEmpty())
    // user canceled out
    return -1;

  // user must have entered an incorrect password
  return 1;
}

// \brief Show gamepad keypad and verify user input against strToVerify.
// \param strToVerify Value to compare against user input.
// \param dlgHeading String shown on dialog title. Converts to localized string if contains a positive integer.
// \param dlgLine0 String shown on dialog line 0. Converts to localized string if contains a positive integer.
// \param dlgLine1 String shown on dialog line 1. Converts to localized string if contains a positive integer.
// \param dlgLine2 String shown on dialog line 2. Converts to localized string if contains a positive integer.
// \param bGetUserInput If set as true and return=true, strToVerify is overwritten with user input string.
// \param bHideInputChars Masks user input as asterisks if set as true.  Currently not yet implemented.
// \return true if successful display and user input. false if unsucessful display, no user input, or canceled editing.
bool CGUIDialogGamepad::ShowAndVerifyInput(CStdString& strToVerify, const CStdString& dlgHeading,
    const CStdString& dlgLine0, const CStdString& dlgLine1,
    const CStdString& dlgLine2, bool bGetUserInput, bool bHideInputChars)
{
  // Prompt user for password input
  CGUIDialogGamepad *pDialog = (CGUIDialogGamepad *)g_windowManager.GetWindow(WINDOW_DIALOG_GAMEPAD);
  pDialog->m_strPassword = strToVerify;
  pDialog->m_bUserInputCleanup = !bGetUserInput;
  pDialog->m_bHideInputChars = bHideInputChars;

  // HACK: This won't work if the label specified is actually a positive numeric value, but that's very unlikely
  if (!StringUtils::IsNaturalNumber(dlgHeading))
    pDialog->SetHeading( dlgHeading );
  else
    pDialog->SetHeading( atoi(dlgHeading.c_str()) );

  if (!StringUtils::IsNaturalNumber(dlgLine0))
    pDialog->SetLine( 0, dlgLine0 );
  else
    pDialog->SetLine( 0, atoi(dlgLine0.c_str()) );

  if (!StringUtils::IsNaturalNumber(dlgLine1))
    pDialog->SetLine( 1, dlgLine1 );
  else
    pDialog->SetLine( 1, atoi(dlgLine1.c_str()) );

  if (!StringUtils::IsNaturalNumber(dlgLine2))
    pDialog->SetLine( 2, dlgLine2 );
  else
    pDialog->SetLine( 2, atoi(dlgLine2.c_str()) );

  g_audioManager.Enable(false); // dont do sounds during pwd input
  pDialog->DoModal();
  g_audioManager.Enable(true);

  if (bGetUserInput && !pDialog->IsCanceled())
  {
    XBMC::XBMC_MD5 md5state;
    md5state.append(pDialog->m_strUserInput);
    md5state.getDigest(strToVerify);
    strToVerify.ToLower();
    pDialog->m_strUserInput = "";
  }

  if (!pDialog->IsConfirmed() || pDialog->IsCanceled())
  {
    // user canceled out or entered an incorrect password
    return false;
  }

  // user entered correct password
  return true;
}

bool CGUIDialogGamepad::IsCanceled() const
{
  return m_bCanceled;
}

