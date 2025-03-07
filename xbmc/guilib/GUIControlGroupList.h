/*!
\file GUIControlGroupList.h
\brief
*/

#pragma once



#include "GUIControlGroup.h"

/*!
 \ingroup controls
 \brief list of controls that is scrollable
 */
class CGUIControlGroupList : public CGUIControlGroup
{
public:
  CGUIControlGroupList(int parentID, int controlID, float posX, float posY, float width, float height, float itemGap, int pageControl, ORIENTATION orientation, bool useControlPositions, uint32_t alignment, const CScroller& scroller);
  virtual ~CGUIControlGroupList(void);
  virtual CGUIControlGroupList *Clone() const { return new CGUIControlGroupList(*this); };

  virtual float GetWidth() const;
  virtual float GetHeight() const;
  virtual float Size() const;

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual bool OnMessage(CGUIMessage& message);

  virtual EVENT_RESULT SendMouseEvent(const CPoint &point, const CMouseEvent &event);
  virtual void UnfocusFromPoint(const CPoint &point);

  virtual void AddControl(CGUIControl *control, int position = -1);
  virtual void ClearAll();

  virtual bool GetCondition(int condition, int data) const;
  /**
   * Calculate total size of child controls area (including gaps between controls)
   */
  float GetTotalSize() const;
  ORIENTATION GetOrientation() const { return m_orientation; }

  // based on grouplist orientation pick one value as minSize;
  void SetMinSize(float minWidth, float minHeight);
protected:
  virtual EVENT_RESULT OnMouseEvent(const CPoint &point, const CMouseEvent &event);
  bool IsFirstFocusableControl(const CGUIControl *control) const;
  bool IsLastFocusableControl(const CGUIControl *control) const;
  void ValidateOffset();
  inline float Size(const CGUIControl *control) const;
  void ScrollTo(float offset);
  float GetAlignOffset() const;

  float m_itemGap;
  int m_pageControl;

  float m_totalSize;

  CScroller m_scroller;

  bool m_useControlPositions;
  ORIENTATION m_orientation;
  uint32_t m_alignment;

  // for autosizing
  float m_minSize;
};

