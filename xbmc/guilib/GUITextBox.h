/*!
\file GUITextBox.h
\brief
*/

#ifndef GUILIB_GUITEXTBOX_H
#define GUILIB_GUITEXTBOX_H

#pragma once



#include "GUITextLayout.h"
#include "GUIControl.h"
#include "GUILabel.h"

/*!
 \ingroup controls
 \brief
 */

class TiXmlNode;

class CGUITextBox : public CGUIControl, public CGUITextLayout
{
public:
  CGUITextBox(int parentID, int controlID, float posX, float posY, float width, float height,
              const CLabelInfo &labelInfo, int scrollTime = 200);
  CGUITextBox(const CGUITextBox &from);
  virtual ~CGUITextBox(void);
  virtual CGUITextBox *Clone() const { return new CGUITextBox(*this); };

  virtual void DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual bool OnMessage(CGUIMessage& message);

  void SetPageControl(int pageControl);

  virtual bool CanFocus() const;
  void SetInfo(const CGUIInfoLabel &info);
  void SetAutoScrolling(const TiXmlNode *node);
  void ResetAutoScrolling();
  CStdString GetLabel(int info) const;

  void Scroll(unsigned int offset);

protected:
  virtual void UpdateVisibility(const CGUIListItem *item = NULL);
  virtual bool UpdateColors();
  virtual void UpdateInfo(const CGUIListItem *item = NULL);
  void UpdatePageControl();
  void ScrollToOffset(int offset, bool autoScroll = false);
  unsigned int GetRows() const;
  int GetCurrentPage() const;

  // offset of text in the control for scrolling
  unsigned int m_offset;
  float m_scrollOffset;
  float m_scrollSpeed;
  int   m_scrollTime;
  unsigned int m_itemsPerPage;
  float m_itemHeight;
  unsigned int m_lastRenderTime;

  CLabelInfo m_label;

  TransformMatrix m_cachedTextMatrix;

  // autoscrolling
  unsigned int m_autoScrollCondition;
  int          m_autoScrollTime;      // time to scroll 1 line (ms)
  int          m_autoScrollDelay;     // delay before scroll (ms)
  unsigned int m_autoScrollDelayTime; // current offset into the delay
  CAnimation *m_autoScrollRepeatAnim;

  int m_pageControl;

  CGUIInfoLabel m_info;
};
#endif
