/*!
\file GUIMoverControl.h
\brief
*/

#ifndef GUILIB_GUIMoverCONTROL_H
#define GUILIB_GUIMoverCONTROL_H

#pragma once



#include "GUITexture.h"
#include "GUIControl.h"

#define ALLOWED_DIRECTIONS_ALL   0
#define ALLOWED_DIRECTIONS_UPDOWN  1
#define ALLOWED_DIRECTIONS_LEFTRIGHT 2

#define DIRECTION_NONE 0
#define DIRECTION_UP 1
#define DIRECTION_DOWN 2
#define DIRECTION_LEFT 3
#define DIRECTION_RIGHT 4

// normal alignment is TOP LEFT 0 = topleft, 1 = topright
#define ALIGN_RIGHT   1
#define ALIGN_BOTTOM  2

/*!
 \ingroup controls
 \brief
 */
class CGUIMoverControl : public CGUIControl
{
public:
  CGUIMoverControl(int parentID, int controlID,
                   float posX, float posY, float width, float height,
                   const CTextureInfo& textureFocus, const CTextureInfo& textureNoFocus);

  virtual ~CGUIMoverControl(void);
  virtual CGUIMoverControl *Clone() const { return new CGUIMoverControl(*this); };

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual void OnUp();
  virtual void OnDown();
  virtual void OnLeft();
  virtual void OnRight();
  virtual void AllocResources();
  virtual void FreeResources(bool immediately = false);
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetInvalid();
  virtual void SetPosition(float posX, float posY);
  void SetLimits(int iX1, int iY1, int iX2, int iY2);
  void SetLocation(int iLocX, int iLocY, bool bSetPosition = true);
  int GetXLocation() const { return m_iLocationX;};
  int GetYLocation() const { return m_iLocationY;};

protected:
  virtual EVENT_RESULT OnMouseEvent(const CPoint &point, const CMouseEvent &event);
  virtual bool UpdateColors();
  bool SetAlpha(unsigned char alpha);
  void UpdateSpeed(int nDirection);
  void Move(int iX, int iY);
  CGUITexture m_imgFocus;
  CGUITexture m_imgNoFocus;
  unsigned int m_frameCounter;
  unsigned int m_lastMoveTime;
  int m_nDirection;
  float m_fSpeed;
  float m_fAnalogSpeed;
  float m_fMaxSpeed;
  float m_fAcceleration;
  int m_iX1, m_iX2, m_iY1, m_iY2;
  int m_iLocationX, m_iLocationY;
};
#endif
