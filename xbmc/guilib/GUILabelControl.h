/*!
\file GUILabelControl.h
\brief
*/

#ifndef GUILIB_GUILABELCONTROL_H
#define GUILIB_GUILABELCONTROL_H

#pragma once



#include "GUIControl.h"
#include "GUILabel.h"

/*!
 \ingroup controls
 \brief
 */
class CGUILabelControl :
      public CGUIControl
{
public:
  CGUILabelControl(int parentID, int controlID, float posX, float posY, float width, float height, const CLabelInfo& labelInfo, bool wrapMultiLine, bool bHasPath);
  virtual ~CGUILabelControl(void);
  virtual CGUILabelControl *Clone() const { return new CGUILabelControl(*this); };

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual void UpdateInfo(const CGUIListItem *item = NULL);
  virtual bool CanFocus() const;
  virtual bool OnMessage(CGUIMessage& message);
  virtual CStdString GetDescription() const;
  virtual float GetWidth() const;
  virtual CRect CalcRenderRegion() const;
 
  const CLabelInfo& GetLabelInfo() const { return m_label.GetLabelInfo(); };
  void SetLabel(const std::string &strLabel);
  void ShowCursor(bool bShow = true);
  void SetCursorPos(int iPos);
  int GetCursorPos() const { return m_iCursorPos;};
  void SetInfo(const CGUIInfoLabel&labelInfo);
  void SetWidthControl(float minWidth, bool bScroll);
  void SetAlignment(uint32_t align);
  void SetHighlight(unsigned int start, unsigned int end);

protected:
  bool UpdateColors();
  CStdString ShortenPath(const CStdString &path);

  CGUILabel m_label;

  bool m_bHasPath;
  bool m_bShowCursor;
  int m_iCursorPos;
  unsigned int m_dwCounter;

  // stuff for autowidth
  bool m_autoWidth;
  float m_minWidth;

  // multi-info stuff
  CGUIInfoLabel m_infoLabel;

  unsigned int m_startHighlight;
  unsigned int m_endHighlight;
};
#endif
