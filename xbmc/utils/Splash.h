#pragma once



#include "StdString.h"
#include "threads/Thread.h"

class CGUITextLayout;
class CGUIImage;

class CSplash : public CThread
{
public:
  CSplash(const CStdString& imageName);
  virtual ~CSplash();

  bool IsRunning();
  bool Start();
  void Stop();

  // In case you don't want to use another thread
  void Show();
  void Show(const CStdString& message);
  void Hide();

private:
  virtual void Process();
  virtual void OnStartup();
  virtual void OnExit();

  float fade;
  CStdString m_ImageName;

  CGUITextLayout* m_messageLayout;
  CGUIImage* m_image;
  bool m_layoutWasLoading;
#ifdef HAS_DX
  D3DGAMMARAMP newRamp;
  D3DGAMMARAMP oldRamp;

#endif
};
