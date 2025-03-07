#pragma once



#include "guilib/GUIDialog.h"

class CGUIDialogNumeric :
      public CGUIDialog
{
public:
  enum INPUT_MODE { INPUT_TIME = 1, INPUT_DATE, INPUT_IP_ADDRESS, INPUT_PASSWORD, INPUT_NUMBER, INPUT_TIME_SECONDS };
  CGUIDialogNumeric(void);
  virtual ~CGUIDialogNumeric(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual bool OnBack(int actionID);
  virtual void FrameMove();

  bool IsConfirmed() const;
  bool IsCanceled() const;

  static bool ShowAndVerifyNewPassword(CStdString& strNewPassword);
  static int ShowAndVerifyPassword(CStdString& strPassword, const CStdString& strHeading, int iRetries);
  static bool ShowAndVerifyInput(CStdString& strPassword, const CStdString& strHeading, bool bGetUserInput);

  void SetHeading(const CStdString &strHeading);
  void SetMode(INPUT_MODE mode, void *initial);
  void GetOutput(void *output);

  static bool ShowAndGetTime(SYSTEMTIME &time, const CStdString &heading);
  static bool ShowAndGetDate(SYSTEMTIME &date, const CStdString &heading);
  static bool ShowAndGetIPAddress(CStdString &IPAddress, const CStdString &heading);
  static bool ShowAndGetNumber(CStdString& strInput, const CStdString &strHeading);
  static bool ShowAndGetSeconds(CStdString& timeString, const CStdString &heading);

protected:
  void OnNumber(unsigned int num);
  void VerifyDate(bool checkYear);
  void OnNext();
  void OnPrevious();
  void OnBackSpace();
  void OnOK();
  void OnCancel();

  bool m_bConfirmed;
  bool m_bCanceled;

  INPUT_MODE m_mode;                // the current input mode
  SYSTEMTIME m_datetime;            // for time and date modes
  WORD m_ip[4];                     // for ip address mode
  unsigned int m_block;             // for time, date, and IP methods.
  unsigned int m_lastblock;
  bool m_dirty;                     // true if the current block has been changed.
  CStdString m_number;              ///< for number or password input
};
